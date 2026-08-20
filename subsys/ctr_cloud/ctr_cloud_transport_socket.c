/*
 * Copyright (c) 2026 HARDWARIO a.s.
 *
 * SPDX-License-Identifier: LicenseRef-HARDWARIO-5-Clause
 *
 * IP transport for ctr_cloud. The cloud protocol is a base64 ASCII
 * request/response datagram, so it maps directly onto a UDP socket; the
 * wire format is identical to the cellular path.
 *
 * A fresh socket per exchange is deliberate. Exchanges are minutes apart on
 * a polling node, the cost is negligible, and it avoids holding a socket
 * across a DHCP lease change or a link bounce.
 */

#include "ctr_cloud_transport.h"

/* Zephyr includes */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

/* Standard includes */
#include <errno.h>
#include <string.h>

LOG_MODULE_REGISTER(ctr_cloud_transport_socket, CONFIG_CTR_CLOUD_LOG_LEVEL);

static int resolve_endpoint(struct sockaddr_in *addr)
{
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(CONFIG_CTR_CLOUD_SOCKET_PORT);

	if (zsock_inet_pton(AF_INET, CONFIG_CTR_CLOUD_SOCKET_HOST, &addr->sin_addr) == 1) {
		return 0;
	}

	struct zsock_addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_DGRAM,
	};
	struct zsock_addrinfo *res = NULL;

	int ret = zsock_getaddrinfo(CONFIG_CTR_CLOUD_SOCKET_HOST, NULL, &hints, &res);

	if (ret) {
		LOG_ERR("Call `zsock_getaddrinfo` failed: %d", ret);
		return -EIO;
	}

	addr->sin_addr = net_sin(res->ai_addr)->sin_addr;
	zsock_freeaddrinfo(res);

	return 0;
}

int ctr_cloud_transport_send_recv(const uint8_t *send_buf, size_t send_len, uint8_t *recv_buf,
				  size_t recv_size, size_t *recv_len, bool rai,
				  k_timeout_t timeout)
{
	/* RAI is a cellular RRC hint with no IP equivalent. */
	ARG_UNUSED(rai);

	struct sockaddr_in addr;
	int ret = resolve_endpoint(&addr);

	if (ret) {
		return ret;
	}

	int sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sock < 0) {
		LOG_ERR("Call `zsock_socket` failed: %d", errno);
		return -errno;
	}

	/* ctr_cloud calls the transport with K_FOREVER (ctr_cloud.c:319) while
	 * holding its own m_lock. Converting that arithmetically is wrong -
	 * K_FOREVER.ticks is (k_ticks_t)-1 - and a recv that never returns wedges
	 * the entire cloud subsystem: every later poll then fails with
	 * "Failed to acquire lock: -11". Observed on the bench 2026-08-19.
	 *
	 * Bound it instead. On UDP, a datagram that has not arrived within this
	 * window is lost, and the caller's own retry is the correct recovery -
	 * blocking forever waiting for it never is. */
	int64_t ms;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		ms = CONFIG_CTR_CLOUD_SOCKET_MAX_RECV_MS;
	} else {
		ms = k_ticks_to_ms_floor64(timeout.ticks);
	}

	if (ms <= 0 || ms > CONFIG_CTR_CLOUD_SOCKET_MAX_RECV_MS) {
		ms = CONFIG_CTR_CLOUD_SOCKET_MAX_RECV_MS;
	}

	struct zsock_timeval tv = {
		.tv_sec = (uint32_t)(ms / 1000),
		.tv_usec = (uint32_t)((ms % 1000) * 1000),
	};

	/* Must not be ignored. SO_RCVTIMEO is compiled out unless
	 * CONFIG_NET_CONTEXT_RCVTIMEO=y (see sockets_inet.c), in which case this
	 * fails with ENOPROTOOPT and the zsock_recv below blocks forever, taking
	 * the ctr_cloud thread with it. Confirmed on the bench 2026-08-19 - it
	 * wedged a shell thread in exactly this way. The Kconfig selects the
	 * symbol so this should not happen, but a silent hang is bad enough that
	 * it is worth failing loudly instead. */
	ret = zsock_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (ret) {
		LOG_ERR("Call `zsock_setsockopt` failed: %d", errno);
		ret = -errno;
		goto out;
	}

	ret = zsock_sendto(sock, send_buf, send_len, 0, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERR("Call `zsock_sendto` failed: %d", errno);
		ret = -errno;
		goto out;
	}

	if (!recv_buf) {
		*recv_len = 0;
		ret = 0;
		goto out;
	}

	ret = zsock_recv(sock, recv_buf, recv_size, 0);
	if (ret < 0) {
		LOG_ERR("Call `zsock_recv` failed: %d", errno);
		ret = -errno;
		goto out;
	}

	*recv_len = (size_t)ret;
	ret = 0;

out:
	zsock_close(sock);

	return ret;
}

int ctr_cloud_transport_enable(void)
{
	/* The IP interface is brought up by the application (DHCP); there is
	 * no equivalent of powering a modem. */
	return 0;
}

int ctr_cloud_transport_wait_for_ready(k_timeout_t timeout)
{
	struct net_if *iface = net_if_get_default();

	if (!iface) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	k_timepoint_t end = sys_timepoint_calc(timeout);

	while (!net_if_is_up(iface) || !net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED)) {
		if (sys_timepoint_expired(end)) {
			LOG_ERR("Timed out waiting for an IPv4 address");
			return -ETIMEDOUT;
		}

		k_sleep(K_MSEC(500));
	}

	return 0;
}
