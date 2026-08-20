#pragma once

#include <stdbool.h>

#include "lvgl.h"

#define AGENT_QUOTA_HISTORY_POINTS 8
#define AGENT_DASHBOARD_PAGE_COUNT 6

void agent_quota_ui_init(lv_display_t *display);
lv_obj_t *agent_quota_ui_get_face_canvas(void);
void agent_quota_ui_set_connection(const char *state, const char *detail, bool ready);
void agent_quota_ui_set_balance(float total, float granted, float topped_up, const char *currency,
                                bool api_available, const float history[AGENT_QUOTA_HISTORY_POINTS]);
void agent_quota_ui_set_clock(const char *time_text, const char *date_text, const char *weekday_text,
                              bool synced, bool is_friday, int days_until_friday);
void agent_quota_ui_set_settings(const char *provider, const char *model, const char *wifi_ssid,
                                 bool wifi_configured, bool api_configured);
void agent_quota_ui_set_network(const char *ip_text, int rssi);
void agent_quota_ui_set_weather(const char *location, float temperature, const char *condition,
                                float wind_speed, bool ready);
