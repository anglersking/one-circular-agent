idf_component_register(
    SRCS "main.c" "electronbot_face_ui.c"
    INCLUDE_DIRS "."
    EMBED_FILES "../assets/electronbot_speak.gif"
    REQUIRES esp_lcd_touch_cst820
)
