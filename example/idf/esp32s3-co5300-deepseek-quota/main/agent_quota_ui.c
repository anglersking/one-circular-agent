#include <stdio.h>

#include "agent_config.h"
#include "agent_quota_ui.h"

#define UI_SCREEN_SIZE 466

static lv_color_t s_bg;
static lv_color_t s_panel;
static lv_color_t s_primary;
static lv_color_t s_accent;
static lv_color_t s_text;
static lv_color_t s_muted;
static lv_color_t s_line;

static lv_obj_t *s_root;
static lv_obj_t *s_face_page;
static lv_obj_t *s_quota_page;
static lv_obj_t *s_face_state;
static lv_obj_t *s_face_detail;
static lv_obj_t *s_quota_state;
static lv_obj_t *s_balance_value;
static lv_obj_t *s_balance_currency;
static lv_obj_t *s_granted_value;
static lv_obj_t *s_topped_up_value;
static lv_obj_t *s_dots[2];
static lv_obj_t *s_eye_pixels[2];
static lv_obj_t *s_chart;
static lv_chart_series_t *s_balance_series;
static int32_t s_press_x;
static uint8_t s_page;
static bool s_blink_closed;

static void palette_init(void)
{
#if AGENT_THEME_AIRPORT
    s_bg = lv_color_hex(0x17191C);
    s_panel = lv_color_hex(0x25292E);
    s_primary = lv_color_hex(0xFFD54A);
    s_accent = lv_color_hex(0xE8EDF2);
    s_text = lv_color_hex(0xF5F7F9);
    s_muted = lv_color_hex(0xAAB4BE);
    s_line = lv_color_hex(0x555E67);
#else
    s_bg = lv_color_hex(0x070B12);
    s_panel = lv_color_hex(0x111927);
    s_primary = lv_color_hex(0x1769FF);
    s_accent = lv_color_hex(0x74F0FF);
    s_text = lv_color_hex(0xF7FAFF);
    s_muted = lv_color_hex(0x94A3BD);
    s_line = lv_color_hex(0x263855);
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

static lv_obj_t *block(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h, lv_color_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);
    make_block(obj, x, y, w, h, color);
    return obj;
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

static void switch_page(uint8_t page)
{
    s_page = page % 2;

    if (s_page == 0) {
        lv_obj_clear_flag(s_face_page, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_quota_page, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_face_page, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_quota_page, LV_OBJ_FLAG_HIDDEN);
    }

    for (uint8_t i = 0; i < 2; ++i) {
        lv_obj_set_style_bg_color(s_dots[i], i == s_page ? s_primary : s_line, LV_PART_MAIN);
    }
}

static void swipe_event_cb(lv_event_t *event)
{
    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) {
        return;
    }

    lv_point_t point;
    lv_indev_get_point(indev, &point);

    if (lv_event_get_code(event) == LV_EVENT_PRESSED) {
        s_press_x = point.x;
        return;
    }

    if (lv_event_get_code(event) != LV_EVENT_RELEASED) {
        return;
    }

    if (point.x < s_press_x - 48) {
        switch_page(1);
    } else if (point.x > s_press_x + 48) {
        switch_page(0);
    }
}

static void blink_timer_cb(lv_timer_t *timer)
{
    s_blink_closed = !s_blink_closed;
    for (uint8_t i = 0; i < 2; ++i) {
        lv_obj_set_y(s_eye_pixels[i], s_blink_closed ? 209 : 199);
        lv_obj_set_height(s_eye_pixels[i], s_blink_closed ? 7 : 27);
    }
    lv_timer_set_period(timer, s_blink_closed ? 150 : 2800);
}

static lv_obj_t *make_page(void)
{
    lv_obj_t *page = lv_obj_create(s_root);
    make_block(page, 0, 0, UI_SCREEN_SIZE, UI_SCREEN_SIZE, s_bg);
    lv_obj_add_flag(page, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(page, swipe_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(page, swipe_event_cb, LV_EVENT_RELEASED, NULL);
    return page;
}

static void make_face_page(void)
{
    s_face_page = make_page();

    label(s_face_page, AGENT_DISPLAY_NAME, 70, 49, 326, &lv_font_montserrat_20, s_text, LV_TEXT_ALIGN_CENTER);
    label(s_face_page, AGENT_MODEL_NAME, 80, 76, 306, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *arc = lv_arc_create(s_face_page);
    lv_obj_remove_style_all(arc);
    lv_obj_set_pos(arc, 49, 88);
    lv_obj_set_size(arc, 368, 368);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 76);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_rotation(arc, 135);
    lv_obj_set_style_arc_width(arc, 11, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, s_line, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 11, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, s_primary, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(arc, 0, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    for (uint8_t i = 0; i < 5; ++i) {
        block(s_face_page, 113 + i * 60, 107, 9, 9, i == 2 ? s_accent : s_primary);
    }

    lv_obj_t *left_eye = block(s_face_page, 127, 183, 84, 61, s_panel);
    lv_obj_set_style_border_width(left_eye, 3, LV_PART_MAIN);
    lv_obj_set_style_border_color(left_eye, s_primary, LV_PART_MAIN);
    lv_obj_t *right_eye = block(s_face_page, 255, 183, 84, 61, s_panel);
    lv_obj_set_style_border_width(right_eye, 3, LV_PART_MAIN);
    lv_obj_set_style_border_color(right_eye, s_primary, LV_PART_MAIN);

    s_eye_pixels[0] = block(s_face_page, 156, 199, 27, 27, s_accent);
    s_eye_pixels[1] = block(s_face_page, 283, 199, 27, 27, s_accent);
    block(s_face_page, 181, 269, 18, 9, s_primary);
    block(s_face_page, 199, 278, 68, 9, s_primary);
    block(s_face_page, 267, 269, 18, 9, s_primary);

    s_face_state = label(s_face_page, "CONFIG REQUIRED", 70, 329, 326, &lv_font_montserrat_20, s_text, LV_TEXT_ALIGN_CENTER);
    s_face_detail = label(s_face_page, "EDIT AGENT_CONFIG.H", 70, 358, 326, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);
    label(s_face_page, "SWIPE FOR BALANCE", 70, 411, 326, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);
}

static void make_quota_page(void)
{
    s_quota_page = make_page();

    label(s_quota_page, "AGENT BALANCE", 60, 49, 346, &lv_font_montserrat_20, s_text, LV_TEXT_ALIGN_CENTER);
    label(s_quota_page, AGENT_MODEL_NAME, 60, 76, 346, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);
    block(s_quota_page, 113, 106, 240, 4, s_primary);

    s_balance_value = label(s_quota_page, "--.--", 50, 122, 366, &lv_font_montserrat_24, s_text, LV_TEXT_ALIGN_CENTER);
    s_balance_currency = label(s_quota_page, "CNY AVAILABLE", 50, 157, 366, &lv_font_montserrat_14, s_primary, LV_TEXT_ALIGN_CENTER);
    s_quota_state = label(s_quota_page, "WAITING FOR SYNC", 50, 185, 366, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);

    label(s_quota_page, "BALANCE HISTORY", 61, 225, 344, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_LEFT);
    s_chart = lv_chart_create(s_quota_page);
    lv_obj_remove_style_all(s_chart);
    lv_obj_set_pos(s_chart, 60, 248);
    lv_obj_set_size(s_chart, 346, 112);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(s_chart, AGENT_QUOTA_HISTORY_POINTS);
    lv_chart_set_div_line_count(s_chart, 3, 0);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_obj_set_style_bg_color(s_chart, s_panel, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_chart, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_chart, s_line, LV_PART_MAIN);
    lv_obj_set_style_radius(s_chart, 0, LV_PART_MAIN);
    lv_obj_set_style_line_color(s_chart, s_line, LV_PART_MAIN);
    lv_obj_set_style_line_width(s_chart, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_chart, 12, LV_PART_MAIN);
    lv_obj_remove_flag(s_chart, LV_OBJ_FLAG_CLICKABLE);
    s_balance_series = lv_chart_add_series(s_chart, s_primary, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_values(s_chart, s_balance_series, 0);

    label(s_quota_page, "OLDER", 61, 366, 90, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_LEFT);
    label(s_quota_page, "NOW", 316, 366, 90, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_RIGHT);
    label(s_quota_page, "GRANT", 82, 397, 120, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);
    label(s_quota_page, "TOP UP", 264, 397, 120, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);
    s_granted_value = label(s_quota_page, "--", 82, 419, 120, &lv_font_montserrat_20, s_text, LV_TEXT_ALIGN_CENTER);
    s_topped_up_value = label(s_quota_page, "--", 264, 419, 120, &lv_font_montserrat_20, s_text, LV_TEXT_ALIGN_CENTER);
}

void agent_quota_ui_init(lv_display_t *display)
{
    palette_init();
    s_root = lv_disp_get_scr_act(display);
    make_block(s_root, 0, 0, UI_SCREEN_SIZE, UI_SCREEN_SIZE, s_bg);

    make_face_page();
    make_quota_page();

    s_dots[0] = block(s_root, 218, 442, 12, 5, s_primary);
    s_dots[1] = block(s_root, 236, 442, 12, 5, s_line);
    switch_page(0);

    lv_timer_create(blink_timer_cb, 2800, NULL);
}

void agent_quota_ui_set_connection(const char *state, const char *detail, bool ready)
{
    if (s_face_state == NULL) {
        return;
    }

    lv_color_t color = ready ? s_primary : s_accent;
    lv_label_set_text(s_face_state, state);
    lv_obj_set_style_text_color(s_face_state, color, LV_PART_MAIN);
    lv_label_set_text(s_face_detail, detail);
    lv_label_set_text(s_quota_state, detail);
    lv_obj_set_style_text_color(s_quota_state, color, LV_PART_MAIN);
}

void agent_quota_ui_set_balance(float total, float granted, float topped_up, const char *currency,
                                bool api_available, const float history[AGENT_QUOTA_HISTORY_POINTS])
{
    char value[32];
    snprintf(value, sizeof(value), "%.2f", total);
    lv_label_set_text(s_balance_value, value);
    snprintf(value, sizeof(value), "%s %s", currency, api_available ? "AVAILABLE" : "UNAVAILABLE");
    lv_label_set_text(s_balance_currency, value);
    snprintf(value, sizeof(value), "%.2f", granted);
    lv_label_set_text(s_granted_value, value);
    snprintf(value, sizeof(value), "%.2f", topped_up);
    lv_label_set_text(s_topped_up_value, value);

    float maximum = 1.0f;
    for (uint8_t i = 0; i < AGENT_QUOTA_HISTORY_POINTS; ++i) {
        if (history[i] > maximum) {
            maximum = history[i];
        }
    }

    int32_t range = (int32_t)(maximum * 120.0f);
    if (range < 100) {
        range = 100;
    }

    int32_t points[AGENT_QUOTA_HISTORY_POINTS];
    for (uint8_t i = 0; i < AGENT_QUOTA_HISTORY_POINTS; ++i) {
        points[i] = (int32_t)(history[i] * 100.0f);
    }

    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 0, range);
    lv_chart_set_series_values(s_chart, s_balance_series, points, AGENT_QUOTA_HISTORY_POINTS);
    lv_chart_refresh(s_chart);
}
