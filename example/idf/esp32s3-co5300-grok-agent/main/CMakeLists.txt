idf_component_register(SRCS "main.c" "grok_agent_ui.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_lcd_touch_cst820 esp_event esp_http_client esp_netif esp-tls esp_wifi json nvs_flash)
