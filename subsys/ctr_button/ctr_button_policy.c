/*
 * Copyright (c) 2026 Belter B.V.
 *
 * SPDX-License-Identifier: LicenseRef-HARDWARIO-5-Clause
 */

#include "ctr_button_policy.h"

/* Standard includes */
#include <string.h>

void ctr_button_policy_stats_init(struct ctr_button_stats *stats)
{
	if (stats->magic != CTR_BUTTON_STATS_MAGIC) {
		memset(stats, 0, sizeof(*stats));
		stats->magic = CTR_BUTTON_STATS_MAGIC;
	}

	stats->boots++;
}

bool ctr_button_policy_clicks_plausible(int clicks, int max_plausible)
{
	return clicks >= 1 && clicks < max_plausible;
}

