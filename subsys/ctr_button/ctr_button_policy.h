/*
 * Copyright (c) 2026 Belter B.V.
 *
 * SPDX-License-Identifier: LicenseRef-HARDWARIO-5-Clause
 */

#ifndef CHESTER_SUBSYS_CTR_BUTTON_POLICY_H_
#define CHESTER_SUBSYS_CTR_BUTTON_POLICY_H_

/* Standard includes */
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pure decision functions for rejecting EMI-induced phantom button events.
 *
 * Kept free of Zephyr state so they compile standalone in the host test. All of
 * them can only ever *discard* an event — none can invent one — so the worst
 * case of a wrong answer here is a missed press, never a spurious action. */

/* Bumped from BTN1 when the coincidence counters left the struct: a stale
 * .noinit block from the previous layout must not be read as the new one. */
#define CTR_BUTTON_STATS_MAGIC 0x42544e33UL /* "BTN3" */

/* Counters that must outlive a reboot: a reboot is this fault's own terminal
 * symptom, so `.bss` counters erase the evidence exactly when the thing being
 * hunted succeeds — and they fail silently, reading 0 when the truth is "zeroed
 * by the event". The owner places this in `.noinit`. Indexed by
 * `enum ctr_button_channel`. */
struct ctr_button_stats {
	uint32_t magic;
	uint32_t boots;
	/* Split by event type: the apps act on clicks and ignore holds, so one
	 * combined "accepted" counter credited events that never reached an
	 * action. */
	uint32_t accepted_click[2];
	uint32_t accepted_hold[2];
	/* Runs long enough to be interference, collapsed to a single click rather
	 * than dropped. */
	uint32_t clamped_burst[2];
};

/* Zero the block when it holds garbage (a true power-on), otherwise keep the
 * counts and record another boot. */
void ctr_button_policy_stats_init(struct ctr_button_stats *stats);

/* Whether a run of @p clicks is plausibly human. Measured phantom runs are
 * bimodal — 1, or a runaway 14-32, with nothing in between — so a run at or
 * above @p max_plausible is noise by construction. */
bool ctr_button_policy_clicks_plausible(int clicks, int max_plausible);

#ifdef __cplusplus
}
#endif

#endif /* CHESTER_SUBSYS_CTR_BUTTON_POLICY_H_ */
