#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "anim_player.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_lv_adapter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "brookesia_face_player.h"

#define LCD_H_RES 466
#define LCD_V_RES 466
#define EYE_CANVAS_W 284
#define EYE_CANVAS_H 126
#define EYE_FPS 30
#define EYE_COUNT 8
#define EYE_AUTOPLAY_INTERVAL_MS 5000

static const char *TAG = "BROOKESIA_FACE";

typedef struct {
    const uint8_t *start;
    const uint8_t *end;
    const char *name;
    uint8_t color_r;
    uint8_t color_g;
    uint8_t color_b;
    bool angry_timeline;
} emotion_asset_t;

extern const uint8_t _binary_emotion_angry_284_126_aaf_start[];
extern const uint8_t _binary_emotion_angry_284_126_aaf_end[];
extern const uint8_t _binary_emotion_blink1_284_126_aaf_start[];
extern const uint8_t _binary_emotion_blink1_284_126_aaf_end[];
extern const uint8_t _binary_emotion_blink_fast_284_126_aaf_start[];
extern const uint8_t _binary_emotion_blink_fast_284_126_aaf_end[];
extern const uint8_t _binary_emotion_blink_slow_284_126_aaf_start[];
extern const uint8_t _binary_emotion_blink_slow_284_126_aaf_end[];
extern const uint8_t _binary_emotion_dizzy_284_126_aaf_start[];
extern const uint8_t _binary_emotion_dizzy_284_126_aaf_end[];
extern const uint8_t _binary_emotion_happy_284_126_aaf_start[];
extern const uint8_t _binary_emotion_happy_284_126_aaf_end[];
extern const uint8_t _binary_emotion_sad_284_126_aaf_start[];
extern const uint8_t _binary_emotion_sad_284_126_aaf_end[];
extern const uint8_t _binary_emotion_sleep_284_126_aaf_start[];
extern const uint8_t _binary_emotion_sleep_284_126_aaf_end[];

static const emotion_asset_t s_emotions[EYE_COUNT] = {
    {_binary_emotion_happy_284_126_aaf_start, _binary_emotion_happy_284_126_aaf_end, "happy", 23, 105, 255, false},
    {_binary_emotion_blink_fast_284_126_aaf_start, _binary_emotion_blink_fast_284_126_aaf_end, "blink-fast", 23, 105, 255, false},
    {_binary_emotion_blink1_284_126_aaf_start, _binary_emotion_blink1_284_126_aaf_end, "blink", 23, 105, 255, false},
    {_binary_emotion_angry_284_126_aaf_start, _binary_emotion_angry_284_126_aaf_end, "angry", 255, 55, 65, true},
    {_binary_emotion_dizzy_284_126_aaf_start, _binary_emotion_dizzy_284_126_aaf_end, "dizzy", 23, 105, 255, false},
    {_binary_emotion_sad_284_126_aaf_start, _binary_emotion_sad_284_126_aaf_end, "sad", 23, 105, 255, false},
    {_binary_emotion_sleep_284_126_aaf_start, _binary_emotion_sleep_284_126_aaf_end, "sleep", 23, 105, 255, false},
    {_binary_emotion_blink_slow_284_126_aaf_start, _binary_emotion_blink_slow_284_126_aaf_end, "blink-slow", 23, 105, 255, false},
};

static anim_player_handle_t s_player;
static lv_display_t *s_display;
static lv_obj_t *s_canvas;
static void *s_canvas_buffer;
static lv_color_t s_background_color;
static SemaphoreHandle_t s_player_mutex;
static QueueHandle_t s_command_queue;
static size_t s_emotion_index;
static volatile uint32_t s_frame_index;
static uint32_t s_frame_count;
static volatile bool s_active;
static volatile bool s_autoplay;

typedef enum {
    FACE_COMMAND_NEXT,
} face_command_t;

static uint8_t blend_channel(uint8_t from, uint8_t to, uint8_t mix)
{
    return (uint8_t)(from + ((int32_t)to - from) * mix / 255);
}

static uint8_t angry_red_mix(uint32_t frame)
{
    // The original Brookesia animation begins with round white eyes, morphs
    // into red angry eyes, then returns to white. Replace that white phase
    // with DeepSeek blue instead of coloring the entire clip red.
    if (frame == 0) {
        return 0;
    }
    if (frame <= 8) {
        return (uint8_t)(frame * 255 / 8);
    }
    if (frame <= 58) {
        return 255;
    }
    if (frame <= 66) {
        return (uint8_t)((66 - frame) * 255 / 8);
    }
    return 0;
}

static void anim_update_cb(anim_player_handle_t handle, player_event_t event)
{
    (void)handle;
    if (event == PLAYER_EVENT_ONE_FRAME_DONE && s_frame_count > 0) {
        s_frame_index = (s_frame_index + 1) % s_frame_count;
    }
}

static void anim_flush_cb(anim_player_handle_t handle, int x1, int y1, int x2, int y2, const void *data)
{
    if (s_canvas == NULL || s_display == NULL || data == NULL || x1 < 0 || y1 < 0 || x2 <= x1 || y2 <= y1 ||
        x2 > EYE_CANVAS_W || y2 > EYE_CANVAS_H) {
        anim_player_flush_ready(handle);
        return;
    }

    // image_player decodes the AAF frame to RGB565. The AAF assets use black as
    // their matte, so treat the grayscale value as alpha and composite the
    // colored eyes over the dashboard background. This removes the rectangular
    // black matte while preserving the antialiased edges.
    if (esp_lv_adapter_lock(-1) != ESP_OK) {
        anim_player_flush_ready(handle);
        return;
    }

    lv_draw_buf_t *draw_buf = lv_canvas_get_draw_buf(s_canvas);
    uint8_t *dst = (uint8_t *)lv_canvas_get_buf(s_canvas);
    const int width = x2 - x1;
    const int height = y2 - y1;
    const size_t src_stride = (size_t)width * sizeof(uint16_t);
    if (draw_buf != NULL && dst != NULL) {
        const emotion_asset_t *asset = &s_emotions[s_emotion_index];
        uint8_t eye_r = asset->color_r;
        uint8_t eye_g = asset->color_g;
        uint8_t eye_b = asset->color_b;
        if (asset->angry_timeline) {
            const uint8_t mix = angry_red_mix(s_frame_index);
            eye_r = blend_channel(23, eye_r, mix);
            eye_g = blend_channel(105, eye_g, mix);
            eye_b = blend_channel(255, eye_b, mix);
        }
        const uint8_t bg_r = s_background_color.red;
        const uint8_t bg_g = s_background_color.green;
        const uint8_t bg_b = s_background_color.blue;
        for (int y = 0; y < height; ++y) {
            const uint16_t *src_row = (const uint16_t *)((const uint8_t *)data + (size_t)y * src_stride);
            uint16_t *dst_row = (uint16_t *)(dst + (size_t)(y1 + y) * draw_buf->header.stride) + x1;
            for (int x = 0; x < width; ++x) {
                uint16_t gray = src_row[x];
                uint8_t r = (uint8_t)(((gray >> 11) & 0x1F) * 255 / 31);
                uint8_t g = (uint8_t)(((gray >> 5) & 0x3F) * 255 / 63);
                uint8_t b = (uint8_t)((gray & 0x1F) * 255 / 31);
                uint8_t luminance = r > g ? r : g;
                if (b > luminance) {
                    luminance = b;
                }
                uint8_t out_r = (uint8_t)(bg_r + ((int32_t)eye_r - bg_r) * luminance / 255);
                uint8_t out_g = (uint8_t)(bg_g + ((int32_t)eye_g - bg_g) * luminance / 255);
                uint8_t out_b = (uint8_t)(bg_b + ((int32_t)eye_b - bg_b) * luminance / 255);
                dst_row[x] = (uint16_t)(((out_r >> 3) << 11) |
                                        ((out_g >> 2) << 5) |
                                        (out_b >> 3));
            }
        }
        lv_obj_invalidate(s_canvas);
    }
    esp_lv_adapter_unlock();
    anim_player_flush_ready(handle);
}

static void clear_face(void)
{
    if (s_canvas == NULL || s_display == NULL || esp_lv_adapter_lock(-1) != ESP_OK) {
        return;
    }
    lv_canvas_fill_bg(s_canvas, s_background_color, LV_OPA_COVER);
    esp_lv_adapter_unlock();
}

static void play_current(void)
{
    const emotion_asset_t *asset = &s_emotions[s_emotion_index];
    uint32_t start = 0;
    uint32_t end = 0;

    ESP_ERROR_CHECK(anim_player_set_src_data(s_player, asset->start, (size_t)(asset->end - asset->start)));
    anim_player_get_segment(s_player, &start, &end);
    s_frame_index = 0;
    s_frame_count = end - start + 1;
    anim_player_set_segment(s_player, start, end, EYE_FPS, true);
    anim_player_update(s_player, PLAYER_ACTION_START);
    ESP_LOGI(TAG, "emotion: %s", asset->name);
}

static void autoplay_task(void *arg)
{
    (void)arg;
    TickType_t last_switch = xTaskGetTickCount();
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(250));
        if (!s_active || !s_autoplay || s_player_mutex == NULL) {
            last_switch = xTaskGetTickCount();
            continue;
        }
        if ((xTaskGetTickCount() - last_switch) < pdMS_TO_TICKS(EYE_AUTOPLAY_INTERVAL_MS)) {
            continue;
        }
        last_switch = xTaskGetTickCount();
        if (xSemaphoreTake(s_player_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            size_t next = (size_t)(esp_random() % EYE_COUNT);
            if (next == s_emotion_index) {
                next = (next + 1) % EYE_COUNT;
            }
            s_emotion_index = next;
            play_current();
            xSemaphoreGive(s_player_mutex);
        }
    }
}

static void command_task(void *arg)
{
    (void)arg;
    face_command_t command;
    while (true) {
        if (xQueueReceive(s_command_queue, &command, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (command == FACE_COMMAND_NEXT &&
            xSemaphoreTake(s_player_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            s_emotion_index = (s_emotion_index + 1) % EYE_COUNT;
            if (s_active) {
                play_current();
            }
            xSemaphoreGive(s_player_mutex);
        }
    }
}

void brookesia_face_player_init(lv_display_t *display, lv_obj_t *canvas)
{
    s_display = display;
    s_canvas = canvas;
    s_canvas_buffer = heap_caps_calloc(EYE_CANVAS_W * EYE_CANVAS_H, sizeof(uint16_t),
                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_canvas_buffer == NULL) {
        s_canvas_buffer = heap_caps_calloc(EYE_CANVAS_W * EYE_CANVAS_H, sizeof(uint16_t), MALLOC_CAP_8BIT);
    }
    ESP_ERROR_CHECK(s_canvas_buffer != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    lv_obj_t *page = lv_obj_get_parent(s_canvas);
    s_background_color = page != NULL ? lv_obj_get_style_bg_color(page, LV_PART_MAIN) : lv_color_black();
    lv_canvas_set_buffer(s_canvas, s_canvas_buffer, EYE_CANVAS_W, EYE_CANVAS_H, LV_COLOR_FORMAT_RGB565);
    lv_canvas_fill_bg(s_canvas, s_background_color, LV_OPA_COVER);
    esp_lv_adapter_unlock();

    s_player_mutex = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(s_player_mutex != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    s_command_queue = xQueueCreate(4, sizeof(face_command_t));
    ESP_ERROR_CHECK(s_command_queue != NULL ? ESP_OK : ESP_ERR_NO_MEM);

    anim_player_config_t config = {
        .flush_cb = anim_flush_cb,
        .update_cb = anim_update_cb,
        .user_data = NULL,
        .flags = {.swap = false},
        .task = ANIM_PLAYER_INIT_CONFIG(),
    };
    s_player = anim_player_init(&config);
    ESP_ERROR_CHECK(s_player != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    xTaskCreate(command_task, "face_command", 4096, NULL, 4, NULL);
    xTaskCreate(autoplay_task, "face_autoplay", 4096, NULL, 4, NULL);
}

void brookesia_face_player_set_active(bool active)
{
    if (s_player == NULL || s_active == active) {
        return;
    }
    s_active = active;
    if (s_active) {
        if (xSemaphoreTake(s_player_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            play_current();
            xSemaphoreGive(s_player_mutex);
        }
    } else {
        if (xSemaphoreTake(s_player_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            anim_player_update(s_player, PLAYER_ACTION_STOP);
            xSemaphoreGive(s_player_mutex);
            clear_face();
        }
    }
}

void brookesia_face_player_next(void)
{
    if (s_player == NULL || s_command_queue == NULL) {
        return;
    }
    const face_command_t command = FACE_COMMAND_NEXT;
    if (xQueueSend(s_command_queue, &command, 0) != pdTRUE) {
        ESP_LOGW(TAG, "emotion command queue full");
    }
}

void brookesia_face_player_set_autoplay(bool enabled)
{
    s_autoplay = enabled;
    ESP_LOGI(TAG, "random emotion autoplay: %s (%d ms)", enabled ? "ON" : "OFF", EYE_AUTOPLAY_INTERVAL_MS);
}
