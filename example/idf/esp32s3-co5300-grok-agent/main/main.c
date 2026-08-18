#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "cJSON.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_lcd_co5300.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_touch.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_lcd_touch_cst820.h"
#include "esp_lv_adapter.h"
#include "lvgl.h"

#include "agent_config.h"
#include "grok_agent_ui.h"
#include "co5300_init_cmds.h"

static const char *TAG = "GROK_AGENT";

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

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAILED_BIT BIT1
#define WIFI_MAX_RETRIES 5
#define HTTP_RESPONSE_SIZE 2048

typedef struct {
    float values[GROK_AGENT_HISTORY_POINTS];
} balance_history_t;

typedef struct {
    float total;
    float granted;
    float topped_up;
    bool available;
    char currency[8];
} balance_result_t;

typedef struct {
    char data[HTTP_RESPONSE_SIZE];
    size_t length;
} http_response_t;

static EventGroupHandle_t s_wifi_events;
static uint8_t s_wifi_retries;
static balance_history_t s_history;

static void co5300_area_rounder_cb(lv_area_t *area, void *user_data)
{
    (void)user_data;
    area->x1 = (area->x1 >> 1) << 1;
    area->y1 = (area->y1 >> 1) << 1;
    area->x2 = ((area->x2 >> 1) << 1) + 1;
    area->y2 = ((area->y2 >> 1) << 1) + 1;
}

static void ui_status(const char *state, const char *detail, bool ready)
{
    if (esp_lv_adapter_lock(pdMS_TO_TICKS(500)) == ESP_OK) {
        grok_agent_ui_set_connection(state, detail, ready);
        esp_lv_adapter_unlock();
    }
}

static void ui_balance(const balance_result_t *result)
{
    if (esp_lv_adapter_lock(pdMS_TO_TICKS(500)) == ESP_OK) {
        grok_agent_ui_set_balance(result->total, result->granted, result->topped_up, result->currency,
                                  result->available, s_history.values);
        esp_lv_adapter_unlock();
    }
}

static bool wifi_is_configured(void)
{
    return AGENT_WIFI_SSID[0] != '\0';
}

static bool api_is_configured(void)
{
    return AGENT_API_KEY[0] != '\0' && AGENT_BALANCE_URL[0] != '\0';
}

static void history_load(void)
{
    nvs_handle_t handle;
    size_t size = sizeof(s_history.values);
    if (nvs_open("quota", NVS_READWRITE, &handle) != ESP_OK) {
        return;
    }

    esp_err_t err = nvs_get_blob(handle, "history", s_history.values, &size);
    if (err != ESP_OK || size != sizeof(s_history.values)) {
        memset(&s_history, 0, sizeof(s_history));
    }
    nvs_close(handle);
}

static void history_add(float value)
{
    memmove(&s_history.values[0], &s_history.values[1],
            sizeof(s_history.values) - sizeof(s_history.values[0]));
    s_history.values[GROK_AGENT_HISTORY_POINTS - 1] = value;

    nvs_handle_t handle;
    if (nvs_open("quota", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_blob(handle, "history", s_history.values, sizeof(s_history.values));
        nvs_commit(handle);
        nvs_close(handle);
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_events, WIFI_CONNECTED_BIT);
        if (s_wifi_retries++ < WIFI_MAX_RETRIES) {
            ui_status("WIFI RETRY", "CONNECTING TO NETWORK", false);
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_wifi_events, WIFI_FAILED_BIT);
            ui_status("WIFI OFFLINE", "CHECK SSID AND PASSWORD", false);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_wifi_retries = 0;
        xEventGroupClearBits(s_wifi_events, WIFI_FAILED_BIT);
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
        ui_status("WIFI ONLINE", "READY TO SYNC", true);
    }
}

static void wifi_start(void)
{
    s_wifi_events = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL));

    wifi_config_t config = {0};
    strlcpy((char *)config.sta.ssid, AGENT_WIFI_SSID, sizeof(config.sta.ssid));
    strlcpy((char *)config.sta.password, AGENT_WIFI_PASSWORD, sizeof(config.sta.password));
    config.sta.pmf_cfg.capable = true;
    config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ui_status("WIFI STARTING", "CONNECTING TO NETWORK", false);
}

static esp_err_t http_event_handler(esp_http_client_event_t *event)
{
    if (event->event_id != HTTP_EVENT_ON_DATA || event->data_len <= 0) {
        return ESP_OK;
    }

    http_response_t *response = event->user_data;
    size_t room = sizeof(response->data) - response->length - 1;
    size_t copy_length = event->data_len < room ? (size_t)event->data_len : room;
    if (copy_length > 0) {
        memcpy(response->data + response->length, event->data, copy_length);
        response->length += copy_length;
        response->data[response->length] = '\0';
    }
    return ESP_OK;
}

static bool json_number(cJSON *object, const char *name, float *value)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, name);
    if (cJSON_IsNumber(item)) {
        *value = (float)item->valuedouble;
        return true;
    }

    if (cJSON_IsString(item) && item->valuestring != NULL) {
        char *end = NULL;
        float parsed = strtof(item->valuestring, &end);
        if (end != item->valuestring && *end == '\0') {
            *value = parsed;
            return true;
        }
    }

    return false;
}

static bool deepseek_fetch_balance(balance_result_t *result)
{
    http_response_t response = {0};
    char authorization[320];
    snprintf(authorization, sizeof(authorization), "Bearer %s", AGENT_API_KEY);

    esp_http_client_config_t config = {
        .url = AGENT_BALANCE_URL,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 12000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = http_event_handler,
        .user_data = &response,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Could not create HTTP client");
        return false;
    }

    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_header(client, "Authorization", authorization);
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200) {
        ESP_LOGW(TAG, "Balance request failed: err=%s status=%d", esp_err_to_name(err), status);
        return false;
    }

    cJSON *root = cJSON_Parse(response.data);
    if (root == NULL) {
        ESP_LOGW(TAG, "Balance response was not valid JSON");
        return false;
    }

    memset(result, 0, sizeof(*result));
    strlcpy(result->currency, "CNY", sizeof(result->currency));
    cJSON *available = cJSON_GetObjectItemCaseSensitive(root, "is_available");
    result->available = cJSON_IsTrue(available);
    cJSON *balances = cJSON_GetObjectItemCaseSensitive(root, "balance_infos");
    cJSON *balance = cJSON_IsArray(balances) ? cJSON_GetArrayItem(balances, 0) : NULL;
    if (balance == NULL) {
        cJSON_Delete(root);
        return false;
    }

    cJSON *currency = cJSON_GetObjectItemCaseSensitive(balance, "currency");
    if (cJSON_IsString(currency) && currency->valuestring != NULL) {
        strlcpy(result->currency, currency->valuestring, sizeof(result->currency));
    }
    json_number(balance, "total_balance", &result->total);
    json_number(balance, "granted_balance", &result->granted);
    json_number(balance, "topped_up_balance", &result->topped_up);
    cJSON_Delete(root);
    return true;
}

static void balance_task(void *arg)
{
    (void)arg;
    const TickType_t poll_delay = pdMS_TO_TICKS((AGENT_POLL_INTERVAL_SECONDS < 60 ? 60 : AGENT_POLL_INTERVAL_SECONDS) * 1000);

    while (true) {
        EventBits_t bits = xEventGroupWaitBits(s_wifi_events, WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
                                               pdFALSE, pdFALSE, pdMS_TO_TICKS(30000));
        if ((bits & WIFI_CONNECTED_BIT) == 0) {
            if ((bits & WIFI_FAILED_BIT) != 0) {
                vTaskDelay(pdMS_TO_TICKS(30000));
            }
            continue;
        }

        if (!api_is_configured()) {
            ui_status("API KEY MISSING", "EDIT AGENT_CONFIG.H", false);
            vTaskDelay(poll_delay);
            continue;
        }

        ui_status("SYNCING", "QUERYING DEEPSEEK BALANCE", true);
        balance_result_t result;
        if (deepseek_fetch_balance(&result)) {
            history_add(result.total);
            ui_balance(&result);
            ESP_LOGI(TAG, "Balance synced: %.2f %s", result.total, result.currency);
            ui_status(result.available ? "BALANCE READY" : "API UNAVAILABLE",
                      result.available ? "SWIPE FOR HISTORY" : "CHECK API ACCOUNT", result.available);
        } else {
            ui_status("SYNC FAILED", "CHECK KEY OR API URL", false);
        }
        vTaskDelay(poll_delay);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting DeepSeek quota agent");
    esp_err_t nvs_error = nvs_flash_init();
    if (nvs_error == ESP_ERR_NVS_NO_FREE_PAGES || nvs_error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_error = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_error);
    history_load();

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
        grok_agent_ui_init(display);
        esp_lv_adapter_unlock();
    }

    if (!wifi_is_configured()) {
        ui_status("CONFIG REQUIRED", "EDIT AGENT_CONFIG.H", false);
        return;
    }

    wifi_start();
    xTaskCreate(balance_task, "balance_sync", 8192, NULL, 5, NULL);
}
