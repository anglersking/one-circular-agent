#pragma once

#include <stdbool.h>

#include "lvgl.h"
#include "agent_config.h"

#define GROK_AGENT_HISTORY_POINTS 8

void grok_agent_ui_init(lv_display_t *display);
void grok_agent_ui_set_connection(const char *state, const char *detail, bool ready);
void grok_agent_ui_set_balance(float total, float granted, float topped_up, const char *currency,
                               bool api_available, const float history[GROK_AGENT_HISTORY_POINTS]);
