/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_log.h"

#include "esp_lcd_touch_cst820.h"

#define POINT_NUM_MAX  (1)
#define DATA_START_REG (0x00)
#define CHIP_ID_REG    (0xA7)

static const char *TAG = "CST820";

static esp_err_t read_data(esp_lcd_touch_handle_t tp);
static bool get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y,
                   uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);
static esp_err_t del(esp_lcd_touch_handle_t tp);
static esp_err_t i2c_read_bytes(esp_lcd_touch_handle_t tp, uint16_t reg,
                                uint8_t *data, uint8_t len);
static esp_err_t reset(esp_lcd_touch_handle_t tp);
static esp_err_t read_id(esp_lcd_touch_handle_t tp);

esp_err_t esp_lcd_touch_new_i2c_cst820(const esp_lcd_panel_io_handle_t io,
                                       const esp_lcd_touch_config_t *config,
                                       esp_lcd_touch_handle_t *tp)
{
    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_ARG, TAG, "Invalid io");
    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "Invalid config");
    ESP_RETURN_ON_FALSE(tp, ESP_ERR_INVALID_ARG, TAG, "Invalid touch handle");

    esp_err_t ret = ESP_OK;
    esp_lcd_touch_handle_t cst820 = calloc(1, sizeof(esp_lcd_touch_t));
    ESP_GOTO_ON_FALSE(cst820, ESP_ERR_NO_MEM, err, TAG, "Touch handle malloc failed");

    cst820->io = io;
    cst820->read_data = read_data;
    cst820->get_xy = get_xy;
    cst820->del = del;
    cst820->data.lock.owner = portMUX_FREE_VAL;
    memcpy(&cst820->config, config, sizeof(esp_lcd_touch_config_t));

    if (cst820->config.int_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t int_gpio_config = {
            .mode = GPIO_MODE_INPUT,
            .intr_type = (cst820->config.levels.interrupt ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE),
            .pin_bit_mask = BIT64(cst820->config.int_gpio_num),
        };
        ESP_GOTO_ON_ERROR(gpio_config(&int_gpio_config), err, TAG, "GPIO intr config failed");
        if (cst820->config.interrupt_callback) {
            esp_lcd_touch_register_interrupt_callback(cst820, cst820->config.interrupt_callback);
        }
    }

    if (cst820->config.rst_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t rst_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = BIT64(cst820->config.rst_gpio_num),
        };
        ESP_GOTO_ON_ERROR(gpio_config(&rst_gpio_config), err, TAG, "GPIO reset config failed");
    }

    ESP_GOTO_ON_ERROR(reset(cst820), err, TAG, "Reset failed");
    ESP_GOTO_ON_ERROR(read_id(cst820), err, TAG, "Read version failed");
    *tp = cst820;

    ESP_LOGI(TAG, "LCD touch panel create success, version: %d.%d.%d",
             ESP_LCD_TOUCH_CST820_VER_MAJOR, ESP_LCD_TOUCH_CST820_VER_MINOR,
             ESP_LCD_TOUCH_CST820_VER_PATCH);
    return ESP_OK;

err:
    if (cst820) {
        del(cst820);
    }
    ESP_LOGE(TAG, "Initialization failed!");
    return ret;
}

static esp_err_t read_data(esp_lcd_touch_handle_t tp)
{
    uint8_t values[15] = {0};
    ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, DATA_START_REG, values, sizeof(values)),
                        TAG, "I2C read failed");

    uint8_t point_num = values[2];
    uint16_t x = (((uint16_t)(values[3] & 0x0f)) << 8) | values[4];
    uint16_t y = (((uint16_t)(values[5] & 0x0f)) << 8) | values[6];

    portENTER_CRITICAL(&tp->data.lock);
    point_num = point_num > POINT_NUM_MAX ? POINT_NUM_MAX : point_num;
    tp->data.points = point_num;
    for (int i = 0; i < point_num; i++) {
        tp->data.coords[i].x = x;
        tp->data.coords[i].y = y;
    }
    portEXIT_CRITICAL(&tp->data.lock);
    return ESP_OK;
}

static bool get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y,
                   uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
{
    portENTER_CRITICAL(&tp->data.lock);
    *point_num = tp->data.points > max_point_num ? max_point_num : tp->data.points;
    for (size_t i = 0; i < *point_num; i++) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;
        if (strength) {
            strength[i] = tp->data.coords[i].strength;
        }
    }
    tp->data.points = 0;
    portEXIT_CRITICAL(&tp->data.lock);
    return *point_num > 0;
}

static esp_err_t del(esp_lcd_touch_handle_t tp)
{
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.int_gpio_num);
        if (tp->config.interrupt_callback) {
            gpio_isr_handler_remove(tp->config.int_gpio_num);
        }
    }
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.rst_gpio_num);
    }
    free(tp);
    return ESP_OK;
}

static esp_err_t reset(esp_lcd_touch_handle_t tp)
{
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num,
                                           tp->config.levels.reset),
                            TAG, "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(200));
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num,
                                           !tp->config.levels.reset),
                            TAG, "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return ESP_OK;
}

static esp_err_t read_id(esp_lcd_touch_handle_t tp)
{
    uint8_t id;
    ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, CHIP_ID_REG, &id, 1), TAG, "I2C read failed");
    ESP_LOGI(TAG, "IC id: %d", id);
    return ESP_OK;
}

static esp_err_t i2c_read_bytes(esp_lcd_touch_handle_t tp, uint16_t reg,
                                uint8_t *data, uint8_t len)
{
    ESP_RETURN_ON_FALSE(data, ESP_ERR_INVALID_ARG, TAG, "Invalid data");
    return esp_lcd_panel_io_rx_param(tp->io, reg, data, len);
}
