#pragma once

#include <stdbool.h>

#include "lvgl.h"

void brookesia_face_player_init(lv_display_t *display, lv_obj_t *canvas);
void brookesia_face_player_set_active(bool active);
void brookesia_face_player_next(void);
void brookesia_face_player_set_autoplay(bool enabled);
