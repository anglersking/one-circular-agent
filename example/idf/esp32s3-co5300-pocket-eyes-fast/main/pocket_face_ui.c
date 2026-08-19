#include "pocket_face_ui.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_lcd_co5300.h"
#include "esp_log.h"
#include "esp_random.h"

#define DISPLAY_SIZE 466
#define NORMAL_WIDTH 96
#define NORMAL_HEIGHT 208
#define CHARGE_WIDTH 128
#define CHARGE_HEIGHT 240
#define EYES_SCALE 440
#define ACTIVE_BRIGHTNESS 80
#define DIM_BRIGHTNESS 6
#define INACTIVITY_TIMEOUT_MS 10000

static const char *TAG = "POCKET_UI";

extern const uint8_t _binary_twece_gif_start[];
extern const uint8_t _binary_twece_gif_end[];
extern const uint8_t _binary_anger_gif_start[];
extern const uint8_t _binary_anger_gif_end[];
extern const uint8_t _binary_disdain_gif_start[];
extern const uint8_t _binary_disdain_gif_end[];
extern const uint8_t _binary_excited_gif_start[];
extern const uint8_t _binary_excited_gif_end[];
extern const uint8_t _binary_once_gif_start[];
extern const uint8_t _binary_once_gif_end[];
extern const uint8_t _binary_charge_gif_start[];
extern const uint8_t _binary_charge_gif_end[];

typedef struct {
    lv_image_dsc_t image;
    const uint8_t *end;
    const char *name;
} pocket_asset_t;

#define POCKET_ASSET(start_symbol, end_symbol, asset_name, asset_width, asset_height) \
    {                                                                    \
        .image = {                                                       \
            .header = {                                                 \
                .magic = LV_IMAGE_HEADER_MAGIC,                         \
                .cf = LV_COLOR_FORMAT_RAW,                              \
                .w = asset_width,                                       \
                .h = asset_height,                                      \
            },                                                          \
            .data = start_symbol,                                       \
        },                                                              \
        .end = end_symbol,                                              \
        .name = asset_name,                                             \
    }

static pocket_asset_t s_faces[] = {
    POCKET_ASSET(_binary_twece_gif_start, _binary_twece_gif_end, "twece", NORMAL_WIDTH, NORMAL_HEIGHT),
    POCKET_ASSET(_binary_anger_gif_start, _binary_anger_gif_end, "anger", NORMAL_WIDTH, NORMAL_HEIGHT),
    POCKET_ASSET(_binary_disdain_gif_start, _binary_disdain_gif_end, "disdain", NORMAL_WIDTH, NORMAL_HEIGHT),
    POCKET_ASSET(_binary_excited_gif_start, _binary_excited_gif_end, "excited", NORMAL_WIDTH, NORMAL_HEIGHT),
    POCKET_ASSET(_binary_once_gif_start, _binary_once_gif_end, "once", NORMAL_WIDTH, NORMAL_HEIGHT),
};

static pocket_asset_t s_charge =
    POCKET_ASSET(_binary_charge_gif_start, _binary_charge_gif_end, "charge", CHARGE_WIDTH, CHARGE_HEIGHT);

#define FACE_COUNT ((uint32_t)(sizeof(s_faces) / sizeof(s_faces[0])))

static lv_obj_t *s_gif;
static esp_lcd_panel_handle_t s_panel;
static uint32_t s_face_index;
static uint32_t s_last_activity_ms;
static bool s_charge_mode;
static bool s_dimmed;
static bool s_skip_release_click;
static bool s_advance_pending;

static void set_brightness(uint8_t brightness)
{
    esp_err_t err = esp_lcd_panel_co5300_set_brightness(s_panel, brightness);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "brightness %u failed: %s", brightness, esp_err_to_name(err));
    }
}

static void load_asset(pocket_asset_t *asset, int32_t loop_count)
{
    lv_gif_set_src(s_gif, &asset->image);
    if (!lv_gif_is_loaded(s_gif)) {
        ESP_LOGE(TAG, "failed to load %s", asset->name);
        return;
    }

    // LVGL uses 256 as 1.0x. The assets were cropped to the expressive eye
    // region, then enlarged moderately so the eyes remain the focal point
    // while leaving a generous black margin around them.
    lv_image_set_scale(s_gif, EYES_SCALE);
    lv_obj_center(s_gif);
    lv_gif_set_loop_count(s_gif, loop_count);
    ESP_LOGI(TAG, "face: %s", asset->name);
}

static void show_normal(uint32_t index)
{
    s_face_index = index % FACE_COUNT;
    s_charge_mode = false;
    s_advance_pending = false;
    load_asset(&s_faces[s_face_index], 1);
}

static void show_random_normal(void)
{
    uint32_t next = esp_random() % (FACE_COUNT - 1U);
    if (next >= s_face_index) {
        next++;
    }
    show_normal(next);
}

static void show_charge(void)
{
    s_charge_mode = true;
    s_advance_pending = false;
    load_asset(&s_charge, 0);
}

static void wake_display(void)
{
    set_brightness(ACTIVE_BRIGHTNESS);
    lv_gif_resume(s_gif);
    s_dimmed = false;
    s_last_activity_ms = lv_tick_get();
    ESP_LOGI(TAG, "display awake");
}

static void gif_ready_cb(lv_event_t *event)
{
    (void)event;
    if (!s_charge_mode) {
        s_advance_pending = true;
    }
}

static void maintenance_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (s_advance_pending && !s_dimmed && !s_charge_mode) {
        show_random_normal();
    }

    if (!s_dimmed && (uint32_t)(lv_tick_get() - s_last_activity_ms) >= INACTIVITY_TIMEOUT_MS) {
        lv_gif_pause(s_gif);
        set_brightness(DIM_BRIGHTNESS);
        s_dimmed = true;
        ESP_LOGI(TAG, "10s inactivity: AMOLED dimmed; tap to wake");
    }
}

static void face_touch_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_PRESSED) {
        if (s_dimmed) {
            wake_display();
            s_skip_release_click = true;
        }
        else {
            s_last_activity_ms = lv_tick_get();
            s_skip_release_click = false;
        }
        return;
    }

    if (code == LV_EVENT_LONG_PRESSED) {
        if (s_skip_release_click) {
            return;
        }
        s_skip_release_click = true;
        if (s_charge_mode) {
            show_random_normal();
        }
        else {
            show_charge();
        }
        return;
    }

    if (code == LV_EVENT_CLICKED) {
        if (s_skip_release_click) {
            s_skip_release_click = false;
            return;
        }
        if (!s_charge_mode) {
            show_normal(s_face_index + 1U);
        }
    }
}

void pocket_face_ui_init(lv_display_t *display, esp_lcd_panel_handle_t panel)
{
    for (uint32_t i = 0; i < FACE_COUNT; ++i) {
        s_faces[i].image.data_size = (size_t)(s_faces[i].end - s_faces[i].image.data);
    }
    s_charge.image.data_size = (size_t)(s_charge.end - s_charge.image.data);
    s_panel = panel;

    lv_obj_t *screen = lv_display_get_screen_active(display);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(screen, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(screen, true, LV_PART_MAIN);

    s_gif = lv_gif_create(screen);
    lv_gif_set_color_format(s_gif, LV_COLOR_FORMAT_RGB565);
    lv_obj_add_event_cb(s_gif, gif_ready_cb, LV_EVENT_READY, NULL);

    lv_obj_t *touch_layer = lv_obj_create(screen);
    lv_obj_remove_style_all(touch_layer);
    lv_obj_set_size(touch_layer, DISPLAY_SIZE, DISPLAY_SIZE);
    lv_obj_center(touch_layer);
    lv_obj_set_style_radius(touch_layer, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(touch_layer, true, LV_PART_MAIN);
    lv_obj_add_flag(touch_layer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(touch_layer, face_touch_cb, LV_EVENT_ALL, NULL);

    s_last_activity_ms = lv_tick_get();
    show_normal(esp_random() % FACE_COUNT);
    lv_timer_create(maintenance_timer_cb, 100, NULL);

    ESP_LOGI(TAG, "tap: next exaggerated eye mood; hold: charge mode; touch wakes after dim");
}
