#pragma once

#include "lvgl.h"

/* Set to 1 for the dark airport palette; this demo has no runtime config. */
#define GROK_ICON_THEME_AIRPORT 0

void grok_agent_ui_init(lv_display_t *display);
