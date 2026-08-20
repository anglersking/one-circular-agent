idf_component_register(SRCS "main.c" "agent_quota_ui.c" "brookesia_face_player.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES
                        "../assets/emotion_angry_284_126.aaf"
                        "../assets/emotion_blink1_284_126.aaf"
                        "../assets/emotion_blink_fast_284_126.aaf"
                        "../assets/emotion_blink_slow_284_126.aaf"
                        "../assets/emotion_dizzy_284_126.aaf"
                        "../assets/emotion_happy_284_126.aaf"
                        "../assets/emotion_sad_284_126.aaf"
                        "../assets/emotion_sleep_284_126.aaf"
                    REQUIRES esp_lcd_touch_cst820 esp_event esp_http_client esp_netif esp-tls esp_wifi json nvs_flash)
