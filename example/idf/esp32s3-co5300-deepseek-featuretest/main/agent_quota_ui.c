#include <stdint.h>
#include <stdio.h>

#include "agent_config.h"
#include "agent_quota_ui.h"

#define UI_SCREEN_SIZE 466

#ifndef AGENT_PROVIDER_NAME
#define AGENT_PROVIDER_NAME "DeepSeek"
#endif

#ifndef AGENT_LOCATION_NAME
#define AGENT_LOCATION_NAME "SHANGHAI"
#endif

extern const uint8_t _binary_electronbot_blue_idle_gif_start[];
extern const uint8_t _binary_electronbot_blue_idle_gif_end[];

static lv_color_t s_bg;
static lv_color_t s_panel;
static lv_color_t s_primary;
static lv_color_t s_accent;
static lv_color_t s_text;
static lv_color_t s_muted;
static lv_color_t s_line;
static lv_color_t s_good;
static lv_color_t s_warning;
static lv_color_t s_error;

static lv_obj_t *s_root;
static lv_obj_t *s_face_page;
static lv_obj_t *s_quota_page;
static lv_obj_t *s_friday_page;
static lv_obj_t *s_calendar_page;
static lv_obj_t *s_weather_page;
static lv_obj_t *s_settings_page;
static lv_obj_t *s_face_state;
static lv_obj_t *s_face_detail;
static lv_obj_t *s_face_clock;
static lv_obj_t *s_face_date;
static lv_obj_t *s_quota_state;
static lv_obj_t *s_balance_value;
static lv_obj_t *s_balance_currency;
static lv_obj_t *s_granted_value;
static lv_obj_t *s_topped_up_value;
static lv_obj_t *s_friday_ring;
static lv_obj_t *s_friday_state;
static lv_obj_t *s_friday_weekday;
static lv_obj_t *s_friday_detail;
static lv_obj_t *s_friday_date;
static lv_obj_t *s_calendar_weekday;
static lv_obj_t *s_calendar_date;
static lv_obj_t *s_calendar_lunar;
static lv_obj_t *s_calendar_huangli;
static lv_obj_t *s_weather_location;
static lv_obj_t *s_weather_temp;
static lv_obj_t *s_weather_condition;
static lv_obj_t *s_weather_wind;
static lv_obj_t *s_weather_network;
static lv_obj_t *s_setting_provider;
static lv_obj_t *s_setting_model;
static lv_obj_t *s_setting_wifi;
static lv_obj_t *s_setting_key;
static lv_obj_t *s_setting_autoplay;
static lv_obj_t *s_setting_hint;
static lv_obj_t *s_interval_buttons[3];
static lv_obj_t *s_dots[AGENT_DASHBOARD_PAGE_COUNT];
static lv_obj_t *s_chart;
static lv_chart_series_t *s_balance_series;
static lv_timer_t *s_auto_rotate_timer;
static bool s_auto_rotate_enabled;
static uint32_t s_auto_rotate_interval_seconds = 5;
static int32_t s_press_x;
static uint8_t s_page;
static lv_image_dsc_t s_blue_face_gif = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RAW,
        .w = UI_SCREEN_SIZE,
        .h = UI_SCREEN_SIZE,
    },
    .data = _binary_electronbot_blue_idle_gif_start,
};

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
    s_good = lv_color_hex(0x55D98A);
    s_warning = lv_color_hex(0xFFD54A);
    s_error = lv_color_hex(0xFF5964);
#else
    s_bg = lv_color_hex(0x070B12);
    s_panel = lv_color_hex(0x111927);
    s_primary = lv_color_hex(0x1769FF);
    s_accent = lv_color_hex(0x74F0FF);
    s_text = lv_color_hex(0xF7FAFF);
    s_muted = lv_color_hex(0x94A3BD);
    s_line = lv_color_hex(0x263855);
    s_good = lv_color_hex(0x38E28A);
    s_warning = lv_color_hex(0xF4C95D);
    s_error = lv_color_hex(0xFF4D5A);
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

static void auto_rotate_update_timer(void);
static void switch_page(uint8_t page);

static void settings_autoplay_event_cb(lv_event_t *event)
{
    lv_obj_t *row = lv_event_get_target(event);
    s_auto_rotate_enabled = !s_auto_rotate_enabled;
    lv_label_set_text(s_setting_autoplay, s_auto_rotate_enabled ? "ON" : "OFF");
    lv_obj_set_style_text_color(s_setting_autoplay, s_auto_rotate_enabled ? s_good : s_muted, LV_PART_MAIN);
    lv_obj_set_style_border_color(row, s_auto_rotate_enabled ? s_primary : s_line, LV_PART_MAIN);
    lv_label_set_text(s_setting_hint, s_auto_rotate_enabled ? "AUTOPLAY ACTIVE" : "AUTOPLAY OFF");
    lv_obj_set_style_text_color(s_setting_hint, s_auto_rotate_enabled ? s_good : s_muted, LV_PART_MAIN);
    auto_rotate_update_timer();
}

static void settings_interval_event_cb(lv_event_t *event)
{
    uint32_t seconds = (uint32_t)(uintptr_t)lv_event_get_user_data(event);
    if (seconds != 2 && seconds != 5 && seconds != 10) {
        return;
    }

    s_auto_rotate_interval_seconds = seconds;
    auto_rotate_update_timer();
}

static void update_interval_button_styles(void)
{
    static const uint32_t intervals[] = {2, 5, 10};
    for (size_t i = 0; i < 3; ++i) {
        bool selected = intervals[i] == s_auto_rotate_interval_seconds;
        lv_obj_set_style_bg_color(s_interval_buttons[i], selected ? s_primary : s_panel, LV_PART_MAIN);
        lv_obj_set_style_border_color(s_interval_buttons[i], selected ? s_primary : s_line, LV_PART_MAIN);
        lv_obj_t *text = lv_obj_get_child(s_interval_buttons[i], 0);
        if (text != NULL) {
            lv_obj_set_style_text_color(text, selected ? s_bg : s_text, LV_PART_MAIN);
        }
    }
}

static lv_obj_t *make_interval_button(int32_t x, const char *text, uint32_t seconds)
{
    lv_obj_t *button = lv_obj_create(s_settings_page);
    make_block(button, x, 359, 50, 32, s_panel);
    lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(button, s_line, LV_PART_MAIN);
    lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(button, settings_interval_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)seconds);
    label(button, text, 0, 7, 50, &lv_font_montserrat_14, s_text, LV_TEXT_ALIGN_CENTER);
    return button;
}

static void auto_rotate_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_auto_rotate_enabled) {
        auto_rotate_update_timer();
        return;
    }
    // Autoplay is a carousel: settings is a normal page, and the next tick
    // wraps back to the face page instead of stopping at the end.
    switch_page((uint8_t)((s_page + 1) % AGENT_DASHBOARD_PAGE_COUNT));
}

static void switch_page(uint8_t page)
{
    s_page = page % AGENT_DASHBOARD_PAGE_COUNT;
    lv_obj_t *pages[AGENT_DASHBOARD_PAGE_COUNT] = {
        s_face_page,
        s_quota_page,
        s_friday_page,
        s_calendar_page,
        s_weather_page,
        s_settings_page,
    };

    for (uint8_t i = 0; i < AGENT_DASHBOARD_PAGE_COUNT; ++i) {
        if (i == s_page) {
            lv_obj_clear_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_set_style_bg_color(s_dots[i], i == s_page ? s_primary : s_line, LV_PART_MAIN);
    }
    auto_rotate_update_timer();
}

static void auto_rotate_update_timer(void)
{
    if (s_auto_rotate_timer == NULL) {
        return;
    }

    lv_timer_set_period(s_auto_rotate_timer, s_auto_rotate_interval_seconds * 1000);
    if (s_auto_rotate_enabled) {
        lv_timer_resume(s_auto_rotate_timer);
        lv_timer_reset(s_auto_rotate_timer);
    } else {
        lv_timer_pause(s_auto_rotate_timer);
    }
    update_interval_button_styles();
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
        switch_page((uint8_t)(s_page + 1));
    } else if (point.x > s_press_x + 48) {
        switch_page((uint8_t)(s_page + AGENT_DASHBOARD_PAGE_COUNT - 1));
    }
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

    lv_obj_t *face_gif = lv_gif_create(s_face_page);
    lv_gif_set_color_format(face_gif, LV_COLOR_FORMAT_RGB565);
    lv_gif_set_src(face_gif, &s_blue_face_gif);
    lv_obj_set_size(face_gif, UI_SCREEN_SIZE, UI_SCREEN_SIZE);
    lv_obj_center(face_gif);
    lv_obj_remove_flag(face_gif, LV_OBJ_FLAG_CLICKABLE);

    // Keep the original dashboard ring as a quiet outer frame. The five decorative
    // blocks were removed; the ring remains behind the status text and around the face.
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

    label(s_face_page, AGENT_DISPLAY_NAME, 70, 49, 326, &lv_font_montserrat_20, s_text, LV_TEXT_ALIGN_CENTER);
    label(s_face_page, AGENT_MODEL_NAME, 80, 76, 306, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);

    s_face_clock = label(s_face_page, "--:--", 70, 287, 326, &lv_font_montserrat_24, s_text, LV_TEXT_ALIGN_CENTER);
    s_face_date = label(s_face_page, "TIME NOT SYNCED", 70, 318, 326, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);
    s_face_state = label(s_face_page, "CONFIG REQUIRED", 70, 345, 326, &lv_font_montserrat_20, s_text, LV_TEXT_ALIGN_CENTER);
    s_face_detail = label(s_face_page, "EDIT AGENT_CONFIG.H", 70, 374, 326, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);
    label(s_face_page, "SWIPE TO EXPLORE", 70, 411, 326, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);
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

static void make_friday_page(void)
{
    s_friday_page = make_page();

    label(s_friday_page, "FRIDAY CHECK", 60, 44, 346, &lv_font_montserrat_20, s_text, LV_TEXT_ALIGN_CENTER);
    label(s_friday_page, "WEEKLY AGENT STATUS", 60, 72, 346, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);

    s_friday_ring = lv_arc_create(s_friday_page);
    lv_obj_remove_style_all(s_friday_ring);
    lv_obj_set_pos(s_friday_ring, 18, 18);
    lv_obj_set_size(s_friday_ring, 430, 430);
    lv_arc_set_range(s_friday_ring, 0, 100);
    lv_arc_set_value(s_friday_ring, 100);
    lv_arc_set_bg_angles(s_friday_ring, 0, 359);
    lv_arc_set_rotation(s_friday_ring, 270);
    lv_obj_set_style_arc_width(s_friday_ring, 9, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_friday_ring, s_line, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_friday_ring, 13, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_friday_ring, s_error, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(s_friday_ring, false, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(s_friday_ring, false, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_friday_ring, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(s_friday_ring, 0, LV_PART_KNOB);
    lv_obj_remove_flag(s_friday_ring, LV_OBJ_FLAG_CLICKABLE);

    s_friday_state = label(s_friday_page, "TIME NOT SYNCED", 70, 184, 326, &lv_font_montserrat_24, s_warning,
                           LV_TEXT_ALIGN_CENTER);
    s_friday_weekday = label(s_friday_page, "WEEKDAY UNKNOWN", 70, 219, 326, &lv_font_montserrat_20, s_text,
                             LV_TEXT_ALIGN_CENTER);
    s_friday_detail = label(s_friday_page, "SYNCING NETWORK TIME", 70, 253, 326, &lv_font_montserrat_14, s_muted,
                            LV_TEXT_ALIGN_CENTER);
    s_friday_date = label(s_friday_page, "---- -- --", 70, 281, 326, &lv_font_montserrat_14, s_muted,
                          LV_TEXT_ALIGN_CENTER);
    label(s_friday_page, "TARGET DAY  /  FRIDAY", 70, 353, 326, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);
    label(s_friday_page, "SWIPE FOR SETTINGS", 70, 411, 326, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_CENTER);
}

static void make_calendar_page(void)
{
    s_calendar_page = make_page();

    label(s_calendar_page, "CALENDAR", 60, 44, 346, &lv_font_montserrat_20, s_text, LV_TEXT_ALIGN_CENTER);
    label(s_calendar_page, "DATE / LUNAR / HUANGLI", 60, 72, 346, &lv_font_montserrat_14, s_muted,
          LV_TEXT_ALIGN_CENTER);
    s_calendar_weekday = label(s_calendar_page, "WEEKDAY UNKNOWN", 60, 118, 346, &lv_font_montserrat_24, s_text,
                               LV_TEXT_ALIGN_CENTER);
    s_calendar_date = label(s_calendar_page, "---- -- --", 60, 155, 346, &lv_font_montserrat_20, s_primary,
                            LV_TEXT_ALIGN_CENTER);

    lv_obj_t *lunar_card = block(s_calendar_page, 55, 222, 356, 58, s_panel);
    lv_obj_set_style_border_width(lunar_card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(lunar_card, s_line, LV_PART_MAIN);
    label(s_calendar_page, "LUNAR", 72, 232, 100, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_LEFT);
    s_calendar_lunar = label(s_calendar_page, "DATA SOURCE PENDING", 176, 232, 216, &lv_font_montserrat_14,
                             s_text, LV_TEXT_ALIGN_RIGHT);

    lv_obj_t *huangli_card = block(s_calendar_page, 55, 292, 356, 58, s_panel);
    lv_obj_set_style_border_width(huangli_card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(huangli_card, s_line, LV_PART_MAIN);
    label(s_calendar_page, "HUANGLI", 72, 302, 100, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_LEFT);
    s_calendar_huangli = label(s_calendar_page, "DATA SOURCE PENDING", 176, 302, 216, &lv_font_montserrat_14,
                               s_text, LV_TEXT_ALIGN_RIGHT);

    label(s_calendar_page, "NTP DATE ACTIVE", 60, 379, 346, &lv_font_montserrat_14, s_good, LV_TEXT_ALIGN_CENTER);
    label(s_calendar_page, "SWIPE FOR WEATHER", 60, 411, 346, &lv_font_montserrat_14, s_muted,
          LV_TEXT_ALIGN_CENTER);
}

static void make_weather_page(void)
{
    s_weather_page = make_page();

    label(s_weather_page, "WEATHER", 60, 44, 346, &lv_font_montserrat_20, s_text, LV_TEXT_ALIGN_CENTER);
    s_weather_location = label(s_weather_page, AGENT_LOCATION_NAME, 60, 76, 346, &lv_font_montserrat_14, s_muted,
                               LV_TEXT_ALIGN_CENTER);
    s_weather_temp = label(s_weather_page, "--.- C", 60, 125, 346, &lv_font_montserrat_24, s_text,
                           LV_TEXT_ALIGN_CENTER);
    s_weather_condition = label(s_weather_page, "WEATHER NOT SYNCED", 60, 164, 346, &lv_font_montserrat_20,
                                s_warning, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *wind_card = block(s_weather_page, 55, 231, 356, 58, s_panel);
    lv_obj_set_style_border_width(wind_card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(wind_card, s_line, LV_PART_MAIN);
    label(s_weather_page, "WIND", 72, 241, 100, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_LEFT);
    s_weather_wind = label(s_weather_page, "--.- KM/H", 176, 241, 216, &lv_font_montserrat_14, s_text,
                           LV_TEXT_ALIGN_RIGHT);

    lv_obj_t *network_card = block(s_weather_page, 55, 301, 356, 58, s_panel);
    lv_obj_set_style_border_width(network_card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(network_card, s_line, LV_PART_MAIN);
    label(s_weather_page, "NETWORK", 72, 311, 100, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_LEFT);
    s_weather_network = label(s_weather_page, "IP --  RSSI --", 176, 311, 216, &lv_font_montserrat_14, s_text,
                              LV_TEXT_ALIGN_RIGHT);

    label(s_weather_page, "OPEN-METEO / NO API KEY", 60, 379, 346, &lv_font_montserrat_14, s_good,
          LV_TEXT_ALIGN_CENTER);
    label(s_weather_page, "SWIPE FOR SETTINGS", 60, 411, 346, &lv_font_montserrat_14, s_muted,
          LV_TEXT_ALIGN_CENTER);
}

static lv_obj_t *make_setting_value(int32_t y, const char *title, const char *value)
{
    lv_obj_t *row = block(s_settings_page, 52, y, 362, 42, s_panel);
    lv_obj_set_style_border_width(row, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(row, s_line, LV_PART_MAIN);
    label(s_settings_page, title, 68, y + 8, 170, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_LEFT);
    return label(s_settings_page, value, 238, y + 8, 158, &lv_font_montserrat_14, s_text, LV_TEXT_ALIGN_RIGHT);
}

static void make_settings_page(void)
{
    s_settings_page = make_page();

    label(s_settings_page, "SETTINGS", 60, 44, 346, &lv_font_montserrat_20, s_text, LV_TEXT_ALIGN_CENTER);
    label(s_settings_page, "ONE AGENT CONTROL CENTER", 60, 72, 346, &lv_font_montserrat_14, s_muted,
          LV_TEXT_ALIGN_CENTER);

    s_setting_provider = make_setting_value(111, "PROVIDER", AGENT_PROVIDER_NAME);
    s_setting_model = make_setting_value(161, "MODEL", AGENT_MODEL_NAME);
    s_setting_wifi = make_setting_value(211, "WI-FI", "NOT SET");
    s_setting_key = make_setting_value(261, "API KEY", "NOT SET");
    lv_obj_t *autoplay_row = block(s_settings_page, 52, 311, 362, 42, s_panel);
    lv_obj_set_style_border_width(autoplay_row, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(autoplay_row, s_line, LV_PART_MAIN);
    lv_obj_add_flag(autoplay_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(autoplay_row, settings_autoplay_event_cb, LV_EVENT_CLICKED, NULL);
    label(s_settings_page, "AUTO PLAY", 68, 319, 170, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_LEFT);
    s_setting_autoplay = label(s_settings_page, "OFF", 238, 319, 158, &lv_font_montserrat_14, s_muted,
                                LV_TEXT_ALIGN_RIGHT);

    label(s_settings_page, "INTERVAL", 68, 368, 150, &lv_font_montserrat_14, s_muted, LV_TEXT_ALIGN_LEFT);
    s_interval_buttons[0] = make_interval_button(236, "2S", 2);
    s_interval_buttons[1] = make_interval_button(292, "5S", 5);
    s_interval_buttons[2] = make_interval_button(348, "10S", 10);

    s_setting_hint = label(s_settings_page, "AUTOPLAY OFF", 60, 399, 346, &lv_font_montserrat_14, s_muted,
                           LV_TEXT_ALIGN_CENTER);
    label(s_settings_page, "SWIPE TO RETURN HOME", 60, 420, 346, &lv_font_montserrat_14, s_muted,
          LV_TEXT_ALIGN_CENTER);
}

void agent_quota_ui_init(lv_display_t *display)
{
    palette_init();
    s_blue_face_gif.data_size = (size_t)(_binary_electronbot_blue_idle_gif_end -
                                         _binary_electronbot_blue_idle_gif_start);
    s_root = lv_disp_get_scr_act(display);
    make_block(s_root, 0, 0, UI_SCREEN_SIZE, UI_SCREEN_SIZE, s_bg);

    make_face_page();
    make_quota_page();
    make_friday_page();
    make_calendar_page();
    make_weather_page();
    make_settings_page();

    const int32_t dot_start_x = 183;
    for (uint8_t i = 0; i < AGENT_DASHBOARD_PAGE_COUNT; ++i) {
        s_dots[i] = block(s_root, dot_start_x + i * 18, 442, 10, 5, i == 0 ? s_primary : s_line);
    }
    s_auto_rotate_timer = lv_timer_create(auto_rotate_timer_cb, s_auto_rotate_interval_seconds * 1000, NULL);
    lv_timer_pause(s_auto_rotate_timer);
    update_interval_button_styles();
    switch_page(0);
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

void agent_quota_ui_set_clock(const char *time_text, const char *date_text, const char *weekday_text,
                              bool synced, bool is_friday, int days_until_friday)
{
    if (s_face_clock == NULL || s_friday_state == NULL) {
        return;
    }

    if (!synced) {
        lv_label_set_text(s_face_clock, "--:--");
        lv_label_set_text(s_face_date, "TIME NOT SYNCED");
        lv_label_set_text(s_friday_state, "TIME NOT SYNCED");
        lv_label_set_text(s_friday_weekday, "WEEKDAY UNKNOWN");
        lv_label_set_text(s_friday_detail, "CONNECT TO SYNC NETWORK TIME");
        lv_label_set_text(s_friday_date, "---- -- --");
        lv_label_set_text(s_calendar_weekday, "WEEKDAY UNKNOWN");
        lv_label_set_text(s_calendar_date, "---- -- --");
        lv_obj_set_style_text_color(s_friday_state, s_warning, LV_PART_MAIN);
        lv_obj_set_style_arc_color(s_friday_ring, s_warning, LV_PART_INDICATOR);
        return;
    }

    lv_label_set_text(s_face_clock, time_text);
    lv_label_set_text(s_face_date, date_text);
    lv_label_set_text(s_friday_weekday, weekday_text);
    lv_label_set_text(s_friday_date, date_text);
    lv_label_set_text(s_calendar_weekday, weekday_text);
    lv_label_set_text(s_calendar_date, date_text);

    lv_color_t state_color = is_friday ? s_good : s_error;
    lv_label_set_text(s_friday_state, is_friday ? "FRIDAY TODAY" : "NOT FRIDAY");
    lv_obj_set_style_text_color(s_friday_state, state_color, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_friday_ring, state_color, LV_PART_INDICATOR);

    char detail[40];
    if (is_friday) {
        snprintf(detail, sizeof(detail), "WEEKEND MODE");
    } else if (days_until_friday == 1) {
        snprintf(detail, sizeof(detail), "1 DAY TO FRIDAY");
    } else {
        snprintf(detail, sizeof(detail), "%d DAYS TO FRIDAY", days_until_friday);
    }
    lv_label_set_text(s_friday_detail, detail);
}

void agent_quota_ui_set_settings(const char *provider, const char *model, const char *wifi_ssid,
                                 bool wifi_configured, bool api_configured)
{
    if (s_setting_provider == NULL) {
        return;
    }

    lv_label_set_text(s_setting_provider, provider != NULL ? provider : "CUSTOM");
    lv_label_set_text(s_setting_model, model != NULL ? model : "UNSPECIFIED");
    lv_label_set_text(s_setting_wifi, wifi_configured && wifi_ssid != NULL ? wifi_ssid : "NOT SET");
    lv_label_set_text(s_setting_key, api_configured ? "KEY READY" : "NOT SET");
    lv_obj_set_style_text_color(s_setting_wifi, wifi_configured ? s_good : s_error, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_setting_key, api_configured ? s_good : s_error, LV_PART_MAIN);
    lv_label_set_text(s_setting_hint, wifi_configured && api_configured ? "CONFIG FILE ACTIVE" : "SETUP REQUIRED");
    lv_obj_set_style_text_color(s_setting_hint, wifi_configured && api_configured ? s_good : s_warning, LV_PART_MAIN);
}

void agent_quota_ui_set_network(const char *ip_text, int rssi)
{
    if (s_weather_network == NULL) {
        return;
    }

    char network[48];
    if (ip_text == NULL || ip_text[0] == '\0') {
        snprintf(network, sizeof(network), "IP --  RSSI --");
    } else {
        snprintf(network, sizeof(network), "IP %s  %d DBM", ip_text, rssi);
    }
    lv_label_set_text(s_weather_network, network);
}

void agent_quota_ui_set_weather(const char *location, float temperature, const char *condition,
                                float wind_speed, bool ready)
{
    if (s_weather_temp == NULL) {
        return;
    }

    if (location != NULL && location[0] != '\0') {
        lv_label_set_text(s_weather_location, location);
    }

    if (!ready) {
        lv_label_set_text(s_weather_temp, "--.- C");
        lv_label_set_text(s_weather_condition, condition != NULL ? condition : "WEATHER NOT SYNCED");
        lv_label_set_text(s_weather_wind, "--.- KM/H");
        lv_obj_set_style_text_color(s_weather_condition, s_warning, LV_PART_MAIN);
        return;
    }

    char value[24];
    snprintf(value, sizeof(value), "%.1f C", temperature);
    lv_label_set_text(s_weather_temp, value);
    snprintf(value, sizeof(value), "%.1f KM/H", wind_speed);
    lv_label_set_text(s_weather_wind, value);
    lv_label_set_text(s_weather_condition, condition != NULL ? condition : "WEATHER READY");
    lv_obj_set_style_text_color(s_weather_condition, s_good, LV_PART_MAIN);
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
