/*
 * Copyright (c) 2026 HARDWARIO a.s.
 *
 * SPDX-License-Identifier: LicenseRef-HARDWARIO-5-Clause
 */

#ifndef CHESTER_SUBSYS_CTR_CLOUD_TRANSPORT_H_
#define CHESTER_SUBSYS_CTR_CLOUD_TRANSPORT_H_

/* Zephyr includes */
#include <zephyr/kernel.h>

/* Standard includes */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Exchange one cloud datagram.
 *
 * The payload is the base64-encoded cloud packet. Blocks until a reply
 * arrives or @p timeout elapses. When @p recv_buf is NULL the caller wants
 * no reply and only the send is performed.
 *
 * @param rai Release Assistance Indication. Meaningful only on cellular,
 *            where it tells the network no reply is expected so the modem
 *            can drop out of RRC-Connected sooner. Ignored by transports
 *            that have no such concept.
 *
 * @return 0 on success, negative errno otherwise.
 */
int ctr_cloud_transport_send_recv(const uint8_t *send_buf, size_t send_len, uint8_t *recv_buf,
				  size_t recv_size, size_t *recv_len, bool rai,
				  k_timeout_t timeout);

/** @brief Bring the transport up. Called once from ctr_cloud_transfer_init(). */
int ctr_cloud_transport_enable(void);

/** @brief Block until the transport can carry traffic. */
int ctr_cloud_transport_wait_for_ready(k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* CHESTER_SUBSYS_CTR_CLOUD_TRANSPORT_H_ */
