#include <assert.h>

#include "freertos/FreeRTOS.h"

#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_co5300.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_touch.h"
#include "esp_log.h"

#include "esp_lcd_touch_cst820.h"
#include "esp_lv_adapter.h"
#include "lvgl.h"

#include "co5300_init_cmds.h"
#include "electronbot_face_ui.h"

static const char *TAG = "ELECTRONBOT_FACE";

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
#define LCD_X_GAP 6
#define LCD_Y_GAP 0

static void co5300_area_rounder_cb(lv_area_t *area, void *user_data)
{
    (void)user_data;
    area->x1 = (area->x1 >> 1) << 1;
    area->y1 = (area->y1 >> 1) << 1;
    area->x2 = ((area->x2 >> 1) << 1) + 1;
    area->y2 = ((area->y2 >> 1) << 1) + 1;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting circular ElectronBot face demo");

    spi_bus_config_t bus_config = CO5300_PANEL_BUS_QSPI_CONFIG(
        LCD_PIN_SCK, LCD_PIN_D0, LCD_PIN_D1, LCD_PIN_D2, LCD_PIN_D3,
        LCD_H_RES * 80 * sizeof(uint16_t));
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = LCD_PIN_CS,
        .dc_gpio_num = -1,
        .spi_mode = 0,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 1,
        .lcd_cmd_bits = 32,
        .lcd_param_bits = 8,
        .flags.quad_mode = true,
    };
    esp_lcd_panel_io_handle_t panel_io = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &panel_io));

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
    esp_lcd_panel_handle_t panel = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_co5300(panel_io, &panel_config, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel, LCD_X_GAP, LCD_Y_GAP));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_co5300_set_brightness(panel, 80));

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
    esp_lcd_touch_handle_t touch = NULL;
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst820(touch_io, &touch_config, &touch));

    esp_lv_adapter_config_t adapter_config = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    adapter_config.stack_in_psram = true;
    ESP_ERROR_CHECK(esp_lv_adapter_init(&adapter_config));
    esp_lv_adapter_display_config_t display_config = ESP_LV_ADAPTER_DISPLAY_SPI_WITH_PSRAM_DEFAULT_CONFIG(
        panel, panel_io, LCD_H_RES, LCD_V_RES, ESP_LV_ADAPTER_ROTATE_0);
    lv_display_t *display = esp_lv_adapter_register_display(&display_config);
    assert(display != NULL);
    ESP_ERROR_CHECK(esp_lv_adapter_set_area_rounder_cb(display, co5300_area_rounder_cb, NULL));

    esp_lv_adapter_touch_config_t touch_config_lvgl = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(display, touch);
    lv_indev_t *indev = esp_lv_adapter_register_touch(&touch_config_lvgl);
    assert(indev != NULL);
    ESP_ERROR_CHECK(esp_lv_adapter_start());

    if (esp_lv_adapter_lock(portMAX_DELAY) == ESP_OK) {
        electronbot_face_ui_init(display);
        esp_lv_adapter_unlock();
    }

    ESP_LOGI(TAG, "White-eye face demo ready: no Wi-Fi, API key, balance or chart");
}
