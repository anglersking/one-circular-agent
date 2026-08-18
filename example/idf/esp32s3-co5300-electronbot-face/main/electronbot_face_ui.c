#include "electronbot_face_ui.h"

#include "esp_log.h"

static const char *TAG = "ELECTRONBOT_FACE";

/* Generated from ElectronBot Standalone's speak.json, then flattened to black. */
extern const uint8_t _binary_electronbot_speak_gif_start[];
extern const uint8_t _binary_electronbot_speak_gif_end[];

static lv_image_dsc_t s_speak_gif = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RAW,
        .w = 466,
        .h = 466,
    },
    .data = _binary_electronbot_speak_gif_start,
};

void electronbot_face_ui_init(lv_display_t *display)
{
    s_speak_gif.data_size = (size_t)(_binary_electronbot_speak_gif_end -
                                     _binary_electronbot_speak_gif_start);

    lv_obj_t *screen = lv_disp_get_scr_act(display);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t *gif = lv_gif_create(screen);
    lv_gif_set_color_format(gif, LV_COLOR_FORMAT_RGB565);
    lv_gif_set_src(gif, &s_speak_gif);
    lv_obj_set_size(gif, 466, 466);
    lv_obj_center(gif);

    if (!lv_gif_is_loaded(gif)) {
        ESP_LOGE(TAG, "failed to load embedded ElectronBot GIF");
        return;
    }

    ESP_LOGI(TAG, "ElectronBot white-eye animation ready (466x466, black background)");
}
