/*
 * Copyright (c) 2026 Belter B.V.
 *
 * SPDX-License-Identifier: LicenseRef-HARDWARIO-5-Clause
 */

/* Tests for the pure button-qualification policy that rejects EMI-induced
 * phantom button events. See
 * `plans/2026-08-07-phantom-button-firmware-mitigation-design.md` in the Belter
 * planning repo for the measured data these thresholds come from. */

#include "ctr_button_policy.h"

#include <zephyr/ztest.h>

#include <string.h>

/* ---- click-burst plausibility ---- */

ZTEST(ctr_button_policy, test_human_click_runs_are_plausible)
{
	for (int clicks = 1; clicks <= 4; clicks++) {
		zassert_true(ctr_button_policy_clicks_plausible(clicks, 5),
			     "a run of %d clicks must be reported", clicks);
	}
}

ZTEST(ctr_button_policy, test_runaway_click_runs_are_rejected)
{
	/* Counts observed on the bench during LTE transmit bursts. */
	zassert_false(ctr_button_policy_clicks_plausible(14, 5), NULL);
	zassert_false(ctr_button_policy_clicks_plausible(17, 5), NULL);
	zassert_false(ctr_button_policy_clicks_plausible(32, 5), NULL);
}

ZTEST(ctr_button_policy, test_burst_threshold_is_inclusive)
{
	zassert_true(ctr_button_policy_clicks_plausible(4, 5), "one below the limit is accepted");
	zassert_false(ctr_button_policy_clicks_plausible(5, 5), "the limit itself is rejected");
}

ZTEST(ctr_button_policy, test_empty_run_is_not_a_press)
{
	zassert_false(ctr_button_policy_clicks_plausible(0, 5), NULL);
	zassert_false(ctr_button_policy_clicks_plausible(-1, 5), NULL);
}

/* ---- cross-channel coincidence ---- */

ZTEST(ctr_button_policy, test_simultaneous_channels_are_coincident)
{
	/* A hand cannot press S1 and the gland button in the same instant. */
	zassert_true(ctr_button_policy_is_coincident(1000, 1000, 100), NULL);
	zassert_true(ctr_button_policy_is_coincident(1099, 1000, 100), NULL);
	zassert_true(ctr_button_policy_is_coincident(1100, 1000, 100), "the window is inclusive");
}

ZTEST(ctr_button_policy, test_separated_channels_are_not_coincident)
{
	zassert_false(ctr_button_policy_is_coincident(1101, 1000, 100), NULL);
	zassert_false(ctr_button_policy_is_coincident(9000, 1000, 100), NULL);
}

ZTEST(ctr_button_policy, test_silent_other_channel_is_not_coincident)
{
	/* 0 means "that channel has never produced an edge since boot". */
	zassert_false(ctr_button_policy_is_coincident(1000, 0, 100), NULL);
}

ZTEST(ctr_button_policy, test_zero_window_disables_the_rule)
{
	zassert_false(ctr_button_policy_is_coincident(1000, 1000, 0), NULL);
}

/* ---- persistent statistics ---- */

ZTEST(ctr_button_policy, test_cold_boot_zeroes_garbage_stats)
{
	struct ctr_button_stats stats;

	memset(&stats, 0xA5, sizeof(stats));

	ctr_button_policy_stats_init(&stats);

	zassert_equal(stats.magic, CTR_BUTTON_STATS_MAGIC, NULL);
	zassert_equal(stats.boots, 1, "the first boot counts as one");
	zassert_equal(stats.accepted[0], 0, NULL);
	zassert_equal(stats.accepted[1], 0, NULL);
	zassert_equal(stats.rejected_burst[0], 0, NULL);
	zassert_equal(stats.rejected_burst[1], 0, NULL);
	zassert_equal(stats.rejected_coincid[0], 0, NULL);
	zassert_equal(stats.rejected_coincid[1], 0, NULL);
}

ZTEST(ctr_button_policy, test_warm_reset_preserves_counts)
{
	struct ctr_button_stats stats;

	memset(&stats, 0xA5, sizeof(stats));
	ctr_button_policy_stats_init(&stats);

	stats.accepted[1] = 7;
	stats.rejected_burst[0] = 3;
	stats.rejected_coincid[1] = 11;

	/* A reboot is this fault's own terminal symptom — the counts have to
	 * survive it, or the detector erases its evidence exactly when the
	 * thing being hunted succeeds. */
	ctr_button_policy_stats_init(&stats);

	zassert_equal(stats.boots, 2, NULL);
	zassert_equal(stats.accepted[1], 7, NULL);
	zassert_equal(stats.rejected_burst[0], 3, NULL);
	zassert_equal(stats.rejected_coincid[1], 11, NULL);
}

ZTEST_SUITE(ctr_button_policy, NULL, NULL, NULL, NULL, NULL);
