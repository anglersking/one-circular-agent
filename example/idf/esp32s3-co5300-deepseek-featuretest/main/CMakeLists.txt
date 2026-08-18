idf_component_register(SRCS "main.c" "agent_quota_ui.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES "../assets/electronbot_blue_idle.gif"
                    REQUIRES esp_lcd_touch_cst820 esp_event esp_http_client esp_netif esp-tls esp_wifi json nvs_flash)
