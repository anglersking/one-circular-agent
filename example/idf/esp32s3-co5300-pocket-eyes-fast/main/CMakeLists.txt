idf_component_register(
    SRCS "main.c" "pocket_face_ui.c"
    INCLUDE_DIRS "."
    EMBED_FILES
        "../assets/twece.gif"
        "../assets/anger.gif"
        "../assets/disdain.gif"
        "../assets/excited.gif"
        "../assets/once.gif"
        "../assets/charge.gif"
    REQUIRES esp_lcd_touch_cst820
)
