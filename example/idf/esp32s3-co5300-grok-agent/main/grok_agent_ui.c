#include <math.h>
#include <stdlib.h>

#include "grok_agent_ui.h"

#define UI_SCREEN_SIZE 466

static lv_color_t s_bg;
static lv_color_t s_ink;
static lv_color_t s_eye_color;
static lv_color_t s_accent;
static lv_color_t s_text;
static lv_color_t s_muted;
static lv_color_t s_line;

static lv_obj_t *s_root;
static lv_obj_t *s_blob;
static lv_obj_t *s_blob_shadow;
static lv_obj_t *s_eye[2];
static lv_obj_t *s_mouth;
static lv_obj_t *s_star_top;
static lv_obj_t *s_star_side;
static lv_obj_t *s_face_mood;
static int32_t s_press_x;
static int32_t s_press_y;
static uint32_t s_motion_tick;

typedef enum {
    FACE_CURIOUS,
    FACE_HAPPY,
    FACE_PLAYFUL,
    FACE_THINKING,
    FACE_LISTENING,
    FACE_IDLE,
} face_mood_t;

static const char *const s_mood_names[] = {
    "CURIOUS", "HAPPY", "PLAYFUL", "THINKING", "LISTENING", "IDLE",
};

static void palette_init(void)
{
#if GROK_ICON_THEME_AIRPORT
    s_bg = lv_color_hex(0x1A1916);
    s_ink = lv_color_hex(0x0B0B0A);
    s_eye_color = lv_color_hex(0xF3EFE6);
    s_accent = lv_color_hex(0xE8D7A4);
    s_text = lv_color_hex(0xF2EEE3);
    s_muted = lv_color_hex(0x9C9588);
    s_line = lv_color_hex(0x4A443B);
#else
    s_bg = lv_color_hex(0xEDE9E0);
    s_ink = lv_color_hex(0x111111);
    s_eye_color = lv_color_hex(0xF8F3E8);
    s_accent = lv_color_hex(0xE5BE58);
    s_text = lv_color_hex(0x1D1B18);
    s_muted = lv_color_hex(0x756E63);
    s_line = lv_color_hex(0xC9C1B4);
#endif
}

static void make_block(lv_obj_t *obj, int32_t x, int32_t y, int32_t w, int32_t h, lv_color_t color)
{
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, int32_t x, int32_t y, int32_t width,
                       const lv_font_t *font, lv_color_t color, lv_text_align_t align)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_width(obj, width);
    lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(obj, color, LV_PART_MAIN);
    lv_obj_set_style_text_align(obj, align, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}

static void set_round_object(lv_obj_t *obj, int32_t x, int32_t y, int32_t w, int32_t h,
                             lv_color_t color, int32_t radius)
{
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, radius, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
}

static void set_eye(uint8_t index, int32_t x, int32_t y, int32_t w, int32_t h)
{
    set_round_object(s_eye[index], x, y, w, h, s_eye_color, LV_RADIUS_CIRCLE);
}

static void face_apply(face_mood_t mood, uint32_t tick)
{
    const float bob = sinf((float)tick * 0.045f) * 3.0f;
    const bool blink = mood == FACE_IDLE && (tick % 140U) >= 134U;
    const int32_t body_x = 108;
    const int32_t body_y = 100 + (int32_t)bob;
    const int32_t body_w = mood == FACE_HAPPY ? 258 : 250;
    const int32_t body_h = mood == FACE_HAPPY ? 262 : 270;

    set_round_object(s_blob_shadow, body_x + 8, body_y + 10, body_w, body_h, s_line, LV_RADIUS_CIRCLE);
    lv_obj_set_style_bg_opa(s_blob_shadow, LV_OPA_30, LV_PART_MAIN);
    set_round_object(s_blob, body_x, body_y, body_w, body_h, s_ink, LV_RADIUS_CIRCLE);

    int32_t eye_y = 198 + (int32_t)bob;
    int32_t left_x = 164;
    int32_t right_x = 250;
    int32_t left_w = 58;
    int32_t right_w = 58;
    int32_t eye_h = 73;

    switch (mood) {
        case FACE_CURIOUS:
            left_w = 48;
            right_w = 68;
            eye_h = 82;
            eye_y -= 5;
            break;
        case FACE_HAPPY:
            left_x = 156;
            right_x = 249;
            left_w = 72;
            right_w = 64;
            eye_h = 43;
            eye_y += 13;
            break;
        case FACE_PLAYFUL:
            left_x = 166;
            right_x = 248;
            left_w = 48;
            right_w = 68;
            eye_h = 68;
            eye_y -= 3;
            break;
        case FACE_THINKING:
            left_x = 160;
            right_x = 250;
            left_w = 68;
            right_w = 58;
            eye_h = 55;
            eye_y += 8;
            break;
        case FACE_LISTENING:
            left_x = 168;
            right_x = 250;
            left_w = 46;
            right_w = 46;
            eye_h = 91;
            eye_y -= 5;
            break;
        case FACE_IDLE:
            break;
    }

    if (mood == FACE_IDLE && blink) {
        eye_h = 8;
        eye_y += 32;
    }
    set_eye(0, left_x, eye_y, left_w, eye_h);
    set_eye(1, right_x, eye_y, right_w, eye_h);

    const int32_t mouth_w = mood == FACE_HAPPY ? 88 : (mood == FACE_THINKING ? 42 : 68);
    const int32_t mouth_h = mood == FACE_IDLE ? 7 : (mood == FACE_HAPPY ? 12 : 10);
    const int32_t mouth_x = (UI_SCREEN_SIZE - mouth_w) / 2;
    const int32_t mouth_y = mood == FACE_IDLE ? 299 : (mood == FACE_HAPPY ? 297 : 302);
    set_round_object(s_mouth, mouth_x, mouth_y + (int32_t)bob, mouth_w, mouth_h, s_eye_color,
                     LV_RADIUS_CIRCLE);

    lv_obj_set_y(s_star_top, 78 + (int32_t)(bob * 0.5f));
    lv_obj_set_y(s_star_side, 178 - (int32_t)(bob * 0.4f));
    lv_label_set_text(s_face_mood, s_mood_names[mood]);
}

static void motion_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    s_motion_tick++;
    const face_mood_t mood = (face_mood_t)((s_motion_tick / 100U) % 6U);
    face_apply(mood, s_motion_tick);
}

static void touch_event_cb(lv_event_t *event)
{
    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) {
        return;
    }

    lv_point_t point;
    lv_indev_get_point(indev, &point);
    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED) {
        s_press_x = point.x;
        s_press_y = point.y;
    }
    else if (code == LV_EVENT_RELEASED && abs(point.x - s_press_x) < 32 &&
             abs(point.y - s_press_y) < 32) {
        s_motion_tick += 100U;
    }
}

void grok_agent_ui_init(lv_display_t *display)
{
    palette_init();
    s_root = lv_disp_get_scr_act(display);
    make_block(s_root, 0, 0, UI_SCREEN_SIZE, UI_SCREEN_SIZE, s_bg);
    lv_obj_add_flag(s_root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_root, touch_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_root, touch_event_cb, LV_EVENT_RELEASED, NULL);

    label(s_root, "ONE / DESKTOP AGENT", 50, 35, 366, &lv_font_montserrat_14, s_muted,
          LV_TEXT_ALIGN_CENTER);
    label(s_root, "LVGL ICON STUDY", 70, 57, 326, &lv_font_montserrat_20, s_text,
          LV_TEXT_ALIGN_CENTER);
    s_face_mood = label(s_root, "CURIOUS", 75, 88, 316, &lv_font_montserrat_14, s_accent,
                        LV_TEXT_ALIGN_CENTER);

    s_blob_shadow = lv_obj_create(s_root);
    make_block(s_blob_shadow, 116, 110, 250, 270, s_line);
    lv_obj_set_style_radius(s_blob_shadow, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_blob_shadow, LV_OPA_30, LV_PART_MAIN);

    s_blob = lv_obj_create(s_root);
    make_block(s_blob, 108, 100, 250, 270, s_ink);
    lv_obj_set_style_radius(s_blob, LV_RADIUS_CIRCLE, LV_PART_MAIN);

    s_eye[0] = lv_obj_create(s_root);
    s_eye[1] = lv_obj_create(s_root);
    make_block(s_eye[0], 164, 198, 58, 73, s_eye_color);
    make_block(s_eye[1], 250, 198, 58, 73, s_eye_color);
    lv_obj_set_style_radius(s_eye[0], LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_radius(s_eye[1], LV_RADIUS_CIRCLE, LV_PART_MAIN);

    s_mouth = lv_obj_create(s_root);
    make_block(s_mouth, 199, 302, 68, 10, s_eye_color);
    lv_obj_set_style_radius(s_mouth, LV_RADIUS_CIRCLE, LV_PART_MAIN);

    s_star_top = label(s_root, "*", 86, 78, 24, &lv_font_montserrat_20, s_accent,
                       LV_TEXT_ALIGN_CENTER);
    s_star_side = label(s_root, "+", 361, 180, 24, &lv_font_montserrat_20, s_accent,
                        LV_TEXT_ALIGN_CENTER);
    label(s_root, "TAP TO CHANGE MOOD", 70, 407, 326, &lv_font_montserrat_14, s_muted,
          LV_TEXT_ALIGN_CENTER);
    label(s_root, "PURE LVGL / NO NETWORK", 70, 431, 326, &lv_font_montserrat_14, s_line,
          LV_TEXT_ALIGN_CENTER);

    face_apply(FACE_CURIOUS, 1);
    lv_timer_create(motion_timer_cb, 50, NULL);
}
