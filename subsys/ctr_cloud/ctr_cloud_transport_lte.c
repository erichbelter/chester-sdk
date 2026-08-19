/*
 * Copyright (c) 2026 HARDWARIO a.s.
 *
 * SPDX-License-Identifier: LicenseRef-HARDWARIO-5-Clause
 *
 * Cellular transport for ctr_cloud. This is the behaviour ctr_cloud has
 * always had, moved behind the transport interface unchanged.
 */

#include "ctr_cloud_transport.h"

/* CHESTER includes */
#include <chester/ctr_lte_v2.h>

/* Zephyr includes */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ctr_cloud_transport_lte, CONFIG_CTR_CLOUD_LOG_LEVEL);

int ctr_cloud_transport_send_recv(const uint8_t *send_buf, size_t send_len, uint8_t *recv_buf,
				  size_t recv_size, size_t *recv_len, bool rai,
				  k_timeout_t timeout)
{
	struct ctr_lte_v2_send_recv_param param = {
		.rai = rai,
		.send_as_string = true,
		.send_buf = (uint8_t *)send_buf,
		.send_len = send_len,
		.recv_buf = recv_buf,
		.recv_size = recv_size,
		.recv_len = recv_len,
		.timeout = timeout,
	};

	int ret = ctr_lte_v2_send_recv(&param);

	if (ret) {
		LOG_ERR("Call `ctr_lte_v2_send_recv` failed: %d", ret);
		return ret;
	}

	return 0;
}

int ctr_cloud_transport_enable(void)
{
	ctr_lte_v2_enable();

	return 0;
}

int ctr_cloud_transport_wait_for_ready(k_timeout_t timeout)
{
	int ret = ctr_lte_v2_wait_for_connected(timeout);

	if (ret) {
		LOG_ERR("Call `ctr_lte_v2_wait_for_connected` failed: %d", ret);
		return ret;
	}

	return 0;
}
