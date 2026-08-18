idf_component_register(
    SRCS "main.c" "electronbot_face_ui.c"
    INCLUDE_DIRS "."
    EMBED_FILES
        "../assets/electronbot_speak.gif"
        "../assets/electronbot_face_surprised.gif"
        "../assets/electronbot_face_happy.gif"
        "../assets/electronbot_face_scared.gif"
        "../assets/electronbot_face_angry.gif"
        "../assets/electronbot_face_busy.gif"
        "../assets/electronbot_face_sad.gif"
        "../assets/electronbot_face_confused.gif"
    REQUIRES esp_lcd_touch_cst820
)
