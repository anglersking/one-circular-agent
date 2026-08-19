#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_co5300.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "anim_player.h"
#include "esp_lcd_touch_cst820.h"

#include "co5300_init_cmds.h"

static const char *TAG = "BROOKESIA_EYES";

#define LCD_PIN_CS GPIO_NUM_14
#define LCD_PIN_SCK GPIO_NUM_9
#define LCD_PIN_D0 GPIO_NUM_10
#define LCD_PIN_D1 GPIO_NUM_11
#define LCD_PIN_D2 GPIO_NUM_12
#define LCD_PIN_D3 GPIO_NUM_13
#define LCD_PIN_RST GPIO_NUM_15

#define TOUCH_PIN_SCL GPIO_NUM_42
#define TOUCH_PIN_SDA GPIO_NUM_41
#define TOUCH_PIN_RST GPIO_NUM_40
#define TOUCH_PIN_INT GPIO_NUM_39

#define LCD_HOST SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_H_RES 466
#define LCD_V_RES 466

// The speaker assets are rendered at their final size. There is no LVGL
// scaling step and no low-resolution GIF enlarged at runtime.
#define EYE_CANVAS_W 284
#define EYE_CANVAS_H 126
#define EYE_CANVAS_X ((LCD_H_RES - EYE_CANVAS_W) / 2)
#define EYE_CANVAS_Y ((LCD_V_RES - EYE_CANVAS_H) / 2)
#define EYE_FPS 30

typedef struct {
    const uint8_t *start;
    const uint8_t *end;
    const char *name;
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

static const emotion_asset_t s_emotions[] = {
    {_binary_emotion_happy_284_126_aaf_start, _binary_emotion_happy_284_126_aaf_end, "happy"},
    {_binary_emotion_blink_fast_284_126_aaf_start, _binary_emotion_blink_fast_284_126_aaf_end, "blink-fast"},
    {_binary_emotion_blink1_284_126_aaf_start, _binary_emotion_blink1_284_126_aaf_end, "blink1"},
    {_binary_emotion_angry_284_126_aaf_start, _binary_emotion_angry_284_126_aaf_end, "angry"},
    {_binary_emotion_dizzy_284_126_aaf_start, _binary_emotion_dizzy_284_126_aaf_end, "dizzy"},
    {_binary_emotion_sad_284_126_aaf_start, _binary_emotion_sad_284_126_aaf_end, "sad"},
    {_binary_emotion_sleep_284_126_aaf_start, _binary_emotion_sleep_284_126_aaf_end, "sleep"},
    {_binary_emotion_blink_slow_284_126_aaf_start, _binary_emotion_blink_slow_284_126_aaf_end, "blink-slow"},
};

static esp_lcd_panel_handle_t s_panel;
static esp_lcd_panel_io_handle_t s_panel_io;
static esp_lcd_touch_handle_t s_touch;
static anim_player_handle_t s_player;
static size_t s_emotion_index;

static bool panel_color_done_cb(esp_lcd_panel_io_handle_t panel_io,
                                esp_lcd_panel_io_event_data_t *edata,
                                void *user_ctx)
{
    (void)panel_io;
    (void)edata;
    anim_player_flush_ready((anim_player_handle_t)user_ctx);
    return false;
}

static void anim_flush_cb(anim_player_handle_t handle, int x1, int y1, int x2, int y2, const void *data)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)anim_player_get_user_data(handle);
    if (panel == NULL || data == NULL) {
        anim_player_flush_ready(handle);
        return;
    }

    // x2/y2 are exclusive in image_player. Keep every frame inside the
    // centered eye canvas, matching the Espressif speaker layout.
    if (x1 < 0 || y1 < 0 || x2 <= x1 || y2 <= y1 || x2 > EYE_CANVAS_W || y2 > EYE_CANVAS_H) {
        ESP_LOGE(TAG, "invalid animation region (%d,%d)-(%d,%d)", x1, y1, x2, y2);
        anim_player_flush_ready(handle);
        return;
    }

    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
        panel, EYE_CANVAS_X + x1, EYE_CANVAS_Y + y1,
        EYE_CANVAS_X + x2, EYE_CANVAS_Y + y2, data));
}

static void clear_display(esp_lcd_panel_handle_t panel)
{
    const size_t lines = 8;
    uint16_t *black = heap_caps_calloc(LCD_H_RES * lines, sizeof(uint16_t), MALLOC_CAP_DMA);
    if (black == NULL) {
        ESP_LOGW(TAG, "could not allocate clear buffer; continuing without full clear");
        return;
    }
    for (int y = 0; y < LCD_V_RES; y += lines) {
        int y2 = y + lines;
        if (y2 > LCD_V_RES) {
            y2 = LCD_V_RES;
        }
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, y, LCD_H_RES, y2, black));
        vTaskDelay(1);
    }
    free(black);
}

static void play_emotion(size_t index)
{
    s_emotion_index = index % (sizeof(s_emotions) / sizeof(s_emotions[0]));
    const emotion_asset_t *asset = &s_emotions[s_emotion_index];
    size_t length = (size_t)(asset->end - asset->start);

    ESP_ERROR_CHECK(anim_player_set_src_data(s_player, asset->start, length));
    uint32_t start = 0;
    uint32_t end = 0;
    anim_player_get_segment(s_player, &start, &end);
    anim_player_set_segment(s_player, start, end, EYE_FPS, true);
    anim_player_update(s_player, PLAYER_ACTION_START);
    ESP_LOGI(TAG, "emotion: %s (%u frames, %d fps)", asset->name, (unsigned)(end - start + 1), EYE_FPS);
}

static void touch_task(void *arg)
{
    (void)arg;
    bool was_pressed = false;
    while (true) {
        if (esp_lcd_touch_read_data(s_touch) != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(25));
            continue;
        }
        esp_lcd_touch_point_data_t point = {0};
        uint8_t count = 0;
        bool pressed = (esp_lcd_touch_get_data(s_touch, &point, &count, 1) == ESP_OK) && (count > 0);

        if (pressed && !was_pressed) {
            play_emotion(s_emotion_index + 1);
        }
        was_pressed = pressed;
        vTaskDelay(pdMS_TO_TICKS(25));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Espressif image_player eye demo");

    spi_bus_config_t bus_config = CO5300_PANEL_BUS_QSPI_CONFIG(
        LCD_PIN_SCK, LCD_PIN_D0, LCD_PIN_D1, LCD_PIN_D2, LCD_PIN_D3,
        LCD_H_RES * 80 * sizeof(uint16_t));
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = CO5300_PANEL_IO_QSPI_CONFIG(LCD_PIN_CS, NULL, NULL);
    io_config.pclk_hz = LCD_PIXEL_CLOCK_HZ;
    io_config.trans_queue_depth = 1;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &s_panel_io));

    co5300_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = lcd_init_cmds_size,
        .flags.use_qspi_interface = 1,
    };
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_RST,
        .rgb_endian = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_co5300(s_panel_io, &panel_config, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(s_panel, 6, 0));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_co5300_set_brightness(s_panel, 80));
    clear_display(s_panel);

    i2c_master_bus_config_t i2c_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = TOUCH_PIN_SCL,
        .sda_io_num = TOUCH_PIN_SDA,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t i2c_bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_config, &i2c_bus));
    esp_lcd_panel_io_i2c_config_t touch_io_config = ESP_LCD_TOUCH_IO_I2C_CST820_CONFIG();
    touch_io_config.scl_speed_hz = 400000;
    esp_lcd_panel_io_handle_t touch_io = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &touch_io_config, &touch_io));
    esp_lcd_touch_config_t touch_config = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = TOUCH_PIN_RST,
        .int_gpio_num = TOUCH_PIN_INT,
        .levels = {.reset = 0, .interrupt = 0},
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst820(touch_io, &touch_config, &s_touch));

    anim_player_config_t player_config = {
        .flush_cb = anim_flush_cb,
        .update_cb = NULL,
        .user_data = s_panel,
        .flags = {.swap = true},
        .task = ANIM_PLAYER_INIT_CONFIG(),
    };
    s_player = anim_player_init(&player_config);
    ESP_ERROR_CHECK(s_player != NULL ? ESP_OK : ESP_ERR_NO_MEM);

    esp_lcd_panel_io_callbacks_t callbacks = {
        .on_color_trans_done = panel_color_done_cb,
    };
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(s_panel_io, &callbacks, s_player));

    play_emotion(0);
    xTaskCreate(touch_task, "touch", 4096, NULL, 4, NULL);
    ESP_LOGI(TAG, "ready: 284x126 centered canvas, 30 fps, tap to change emotion");
}
