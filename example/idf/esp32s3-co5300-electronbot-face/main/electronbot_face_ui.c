#include "electronbot_face_ui.h"

#include "esp_log.h"

#define FACE_WIDTH 466
#define FACE_HEIGHT 466

static const char *TAG = "ELECTRONBOT_FACE";
static lv_obj_t *s_gif;
static uint32_t s_face_index;

extern const uint8_t _binary_electronbot_speak_gif_start[];
extern const uint8_t _binary_electronbot_speak_gif_end[];
extern const uint8_t _binary_electronbot_face_surprised_gif_start[];
extern const uint8_t _binary_electronbot_face_surprised_gif_end[];
extern const uint8_t _binary_electronbot_face_happy_gif_start[];
extern const uint8_t _binary_electronbot_face_happy_gif_end[];
extern const uint8_t _binary_electronbot_face_scared_gif_start[];
extern const uint8_t _binary_electronbot_face_scared_gif_end[];
extern const uint8_t _binary_electronbot_face_angry_gif_start[];
extern const uint8_t _binary_electronbot_face_angry_gif_end[];
extern const uint8_t _binary_electronbot_face_busy_gif_start[];
extern const uint8_t _binary_electronbot_face_busy_gif_end[];
extern const uint8_t _binary_electronbot_face_sad_gif_start[];
extern const uint8_t _binary_electronbot_face_sad_gif_end[];
extern const uint8_t _binary_electronbot_face_confused_gif_start[];
extern const uint8_t _binary_electronbot_face_confused_gif_end[];

typedef struct {
    lv_image_dsc_t image;
    const uint8_t *end;
    const char *name;
} face_asset_t;

#define FACE_ASSET(start_symbol, end_symbol, mood_name)                  \
    {                                                                    \
        .image = {                                                       \
            .header = {                                                 \
                .magic = LV_IMAGE_HEADER_MAGIC,                         \
                .cf = LV_COLOR_FORMAT_RAW,                              \
                .w = FACE_WIDTH,                                        \
                .h = FACE_HEIGHT,                                       \
            },                                                          \
            .data = start_symbol,                                       \
        },                                                              \
        .end = end_symbol,                                              \
        .name = mood_name,                                              \
    }

static face_asset_t s_faces[] = {
    FACE_ASSET(_binary_electronbot_speak_gif_start,
               _binary_electronbot_speak_gif_end, "idle"),
    FACE_ASSET(_binary_electronbot_face_surprised_gif_start,
               _binary_electronbot_face_surprised_gif_end, "surprised"),
    FACE_ASSET(_binary_electronbot_face_happy_gif_start,
               _binary_electronbot_face_happy_gif_end, "happy"),
    FACE_ASSET(_binary_electronbot_face_scared_gif_start,
               _binary_electronbot_face_scared_gif_end, "scared"),
    FACE_ASSET(_binary_electronbot_face_angry_gif_start,
               _binary_electronbot_face_angry_gif_end, "angry"),
    FACE_ASSET(_binary_electronbot_face_busy_gif_start,
               _binary_electronbot_face_busy_gif_end, "busy"),
    FACE_ASSET(_binary_electronbot_face_sad_gif_start,
               _binary_electronbot_face_sad_gif_end, "sad"),
    FACE_ASSET(_binary_electronbot_face_confused_gif_start,
               _binary_electronbot_face_confused_gif_end, "confused"),
};

#define FACE_COUNT ((uint32_t)(sizeof(s_faces) / sizeof(s_faces[0])))

static void show_face(uint32_t index)
{
    s_face_index = index % FACE_COUNT;
    lv_gif_set_src(s_gif, &s_faces[s_face_index].image);
    if (!lv_gif_is_loaded(s_gif)) {
        ESP_LOGE(TAG, "failed to load %s face", s_faces[s_face_index].name);
        return;
    }
    ESP_LOGI(TAG, "face: %s (%lu/%u)", s_faces[s_face_index].name,
             (unsigned long)(s_face_index + 1U), (unsigned int)FACE_COUNT);
}

static void face_touch_cb(lv_event_t *event)
{
    (void)event;
    show_face(s_face_index + 1U);
}

void electronbot_face_ui_init(lv_display_t *display)
{
    for (uint32_t i = 0; i < FACE_COUNT; ++i) {
        s_faces[i].image.data_size = (size_t)(s_faces[i].end - s_faces[i].image.data);
    }

    lv_obj_t *screen = lv_disp_get_scr_act(display);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    // The panel is addressed as a 466x466 framebuffer, but its visible area is circular.
    // Keep child drawing and touch hit testing inside that same circle for previews and
    // for boards that expose the full framebuffer to a square capture window.
    lv_obj_set_style_radius(screen, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(screen, true, LV_PART_MAIN);

    s_gif = lv_gif_create(screen);
    lv_gif_set_color_format(s_gif, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_size(s_gif, FACE_WIDTH, FACE_HEIGHT);
    lv_obj_center(s_gif);
    show_face(0);

    lv_obj_t *touch_layer = lv_obj_create(screen);
    lv_obj_remove_style_all(touch_layer);
    lv_obj_set_size(touch_layer, FACE_WIDTH, FACE_HEIGHT);
    lv_obj_center(touch_layer);
    lv_obj_set_style_radius(touch_layer, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(touch_layer, true, LV_PART_MAIN);
    lv_obj_add_flag(touch_layer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(touch_layer, face_touch_cb, LV_EVENT_CLICKED, NULL);

    ESP_LOGI(TAG, "tap screen to cycle 8 smooth white-eye moods; no mouth");
}
