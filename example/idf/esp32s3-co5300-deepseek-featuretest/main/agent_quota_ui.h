#pragma once

#include <stdbool.h>

#include "lvgl.h"

#define AGENT_QUOTA_HISTORY_POINTS 8

void agent_quota_ui_init(lv_display_t *display);
void agent_quota_ui_set_connection(const char *state, const char *detail, bool ready);
void agent_quota_ui_set_balance(float total, float granted, float topped_up, const char *currency,
                                bool api_available, const float history[AGENT_QUOTA_HISTORY_POINTS]);
