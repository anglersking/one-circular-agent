idf_component_register(
    SRCS "main.c"
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
    REQUIRES image_player esp_lcd_touch_cst820
)
