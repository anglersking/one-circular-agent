#include <math.h>
#include <stdlib.h>

#include "grok_agent_ui.h"

#define UI_SCREEN_SIZE 466
#define BODY_POINTS 40
#define EYE_POINTS 14
#define MOUTH_POINTS 18
#define PI_F 3.14159265358979323846f

static lv_color_t s_bg;
static lv_color_t s_ink;
static lv_color_t s_eye_color;
static lv_color_t s_accent;
static lv_color_t s_line;
static lv_obj_t *s_face;
static int32_t s_press_x;
static int32_t s_press_y;
static uint32_t s_motion_tick;

typedef enum {
    FACE_CURIOUS,
    FACE_HAPPY,
    FACE_PLAYFUL,
    FACE_THINKING,
    FACE_LISTENING,
    FACE_IDLE,
} face_mood_t;

typedef struct {
    float width;
    float height;
    float x;
    float y;
    float slant;
} eye_params_t;

static void palette_init(void)
{
#if GROK_ICON_THEME_AIRPORT
    s_bg = lv_color_hex(0x1A1916);
    s_ink = lv_color_hex(0x0B0B0A);
    s_eye_color = lv_color_hex(0xF3EFE6);
    s_accent = lv_color_hex(0xE8D7A4);
    s_line = lv_color_hex(0x4A443B);
#else
    s_bg = lv_color_hex(0xEDE9E0);
    s_ink = lv_color_hex(0x111111);
    s_eye_color = lv_color_hex(0xF8F3E8);
    s_accent = lv_color_hex(0xE5BE58);
    s_line = lv_color_hex(0xC9C1B4);
#endif
}

static float smoothstep(float value)
{
    value = LV_CLAMP(0.0f, value, 1.0f);
    return value * value * (3.0f - 2.0f * value);
}

static void draw_polygon(lv_layer_t *layer, const lv_point_precise_t *points, uint32_t count,
                         lv_point_precise_t center, lv_color_t color, lv_opa_t opa)
{
    lv_draw_triangle_dsc_t triangle;
    lv_draw_triangle_dsc_init(&triangle);
    triangle.color = color;
    triangle.opa = opa;
    triangle.p[0] = center;

    for (uint32_t i = 0; i < count; ++i) {
        triangle.p[1] = points[i];
        triangle.p[2] = points[(i + 1U) % count];
        lv_draw_triangle(layer, &triangle);
    }
}

static void body_points(lv_point_precise_t *points, float center_x, float center_y,
                        float scale_x, float scale_y, float rotation, float phase)
{
    const float radius = 181.0f;
    const float cos_rotation = cosf(rotation);
    const float sin_rotation = sinf(rotation);

    for (uint32_t i = 0; i < BODY_POINTS; ++i) {
        const float angle = 2.0f * PI_F * (float)i / (float)BODY_POINTS;
        const float wobble = 1.0f + 0.035f * sinf(angle * 3.0f + phase)
                             + 0.022f * cosf(angle * 5.0f - phase * 0.7f);
        const float raw_x = cosf(angle) * radius * scale_x * wobble;
        const float raw_y = sinf(angle) * radius * scale_y * wobble;
        points[i].x = center_x + raw_x * cos_rotation - raw_y * sin_rotation;
        points[i].y = center_y + raw_x * sin_rotation + raw_y * cos_rotation;
    }
}

static eye_params_t eye_shape(face_mood_t mood, uint32_t index)
{
    static const eye_params_t shapes[6][2] = {
        {{57.0f, 103.0f, -51.0f, -1.0f, -0.12f}, {70.0f, 83.0f, 51.0f, 1.0f, 0.10f}},
        {{86.0f, 53.0f, -49.0f, 1.0f, -0.04f}, {78.0f, 53.0f, 49.0f, 1.0f, 0.04f}},
        {{52.0f, 82.0f, -54.0f, -5.0f, -0.18f}, {73.0f, 102.0f, 48.0f, 2.0f, 0.16f}},
        {{76.0f, 58.0f, -50.0f, 8.0f, -0.18f}, {64.0f, 66.0f, 49.0f, 4.0f, 0.12f}},
        {{46.0f, 112.0f, -48.0f, -3.0f, -0.03f}, {46.0f, 112.0f, 48.0f, -3.0f, 0.03f}},
        {{63.0f, 91.0f, -50.0f, 1.0f, -0.08f}, {63.0f, 91.0f, 50.0f, 1.0f, 0.08f}},
    };
    return shapes[mood][index];
}

static eye_params_t lerp_eye(eye_params_t a, eye_params_t b, float amount)
{
    eye_params_t result = {
        .width = a.width + (b.width - a.width) * amount,
        .height = a.height + (b.height - a.height) * amount,
        .x = a.x + (b.x - a.x) * amount,
        .y = a.y + (b.y - a.y) * amount,
        .slant = a.slant + (b.slant - a.slant) * amount,
    };
    return result;
}

static void eye_points(lv_point_precise_t *points, eye_params_t params, uint32_t index,
                       float center_x, float center_y, float gaze_x, float gaze_y,
                       float phase, float blink)
{
    const float eye_width = params.width;
    const float eye_height = params.height * blink;
    const float eye_center_x = center_x + params.x + gaze_x;
    const float eye_center_y = center_y + params.y + gaze_y;

    for (uint32_t i = 0; i < EYE_POINTS; ++i) {
        const float angle = 2.0f * PI_F * (float)i / (float)EYE_POINTS;
        const float wobble = 1.0f + 0.075f * sinf(angle * 3.0f + phase + (float)index)
                             + 0.035f * cosf(angle * 5.0f - phase * 0.6f);
        const float x = cosf(angle) * eye_width * 0.5f * wobble;
        const float y = sinf(angle) * eye_height * 0.5f * wobble;
        points[i].x = eye_center_x + x;
        points[i].y = eye_center_y + y + params.slant * x;
    }
}

static void mouth_points(lv_point_precise_t *points, float center_x, float center_y,
                         float width, float height, float phase)
{
    for (uint32_t i = 0; i < MOUTH_POINTS; ++i) {
        const float angle = 2.0f * PI_F * (float)i / (float)MOUTH_POINTS;
        const float wobble = 1.0f + 0.04f * sinf(angle * 3.0f + phase);
        points[i].x = center_x + cosf(angle) * width * 0.5f * wobble;
        points[i].y = center_y + sinf(angle) * height * 0.5f * wobble;
    }
}

static void face_draw_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_DRAW_MAIN) {
        return;
    }

    lv_layer_t *layer = lv_event_get_layer(event);
    const float tick = (float)s_motion_tick;
    const float cycle = tick / 36.0f;
    const face_mood_t mood = (face_mood_t)((uint32_t)cycle % 6U);
    const face_mood_t next_mood = (face_mood_t)(((uint32_t)mood + 1U) % 6U);
    const float transition = smoothstep(fmodf(cycle, 1.0f) / 0.24f);
    const float phase = tick * 0.075f;
    const float bob = sinf(tick * 0.055f) * 5.0f;
    const float center_x = 233.0f + sinf(tick * 0.031f) * 3.0f;
    const float center_y = 235.0f + bob;
    const float scale_x = 0.98f + 0.035f * sinf(tick * 0.043f);
    const float scale_y = 1.02f + 0.035f * cosf(tick * 0.051f);
    const float rotation = 0.035f * sinf(tick * 0.037f);

    lv_point_precise_t body[BODY_POINTS];
    body_points(body, center_x + 7.0f, center_y + 9.0f, scale_x, scale_y, rotation, phase);
    lv_point_precise_t body_center = {.x = center_x + 7.0f, .y = center_y + 9.0f};
    draw_polygon(layer, body, BODY_POINTS, body_center, s_line, LV_OPA_30);

    body_points(body, center_x, center_y, scale_x, scale_y, rotation, phase);
    body_center.x = center_x;
    body_center.y = center_y;
    draw_polygon(layer, body, BODY_POINTS, body_center, s_ink, LV_OPA_COVER);

    const bool blink = (s_motion_tick % 180U) >= 174U;
    const float blink_amount = blink ? 0.08f : 1.0f;
    for (uint32_t i = 0; i < 2; ++i) {
        eye_params_t current = eye_shape(mood, i);
        eye_params_t next = eye_shape(next_mood, i);
        eye_params_t params = lerp_eye(current, next, transition);
        const float gaze_x = sinf(tick * 0.065f + (float)i * 1.7f) * 5.0f;
        const float gaze_y = cosf(tick * 0.047f + (float)i) * 3.0f;
        lv_point_precise_t eye[EYE_POINTS];
        eye_points(eye, params, i, center_x, center_y, gaze_x, gaze_y, phase, blink_amount);
        lv_point_precise_t eye_center = {
            .x = center_x + params.x + gaze_x,
            .y = center_y + params.y + gaze_y,
        };
        draw_polygon(layer, eye, EYE_POINTS, eye_center, s_eye_color, LV_OPA_COVER);
    }

    const float mouth_width = 58.0f + 32.0f * sinf(transition * PI_F);
    const float mouth_height = 9.0f + 6.0f * sinf(transition * PI_F);
    lv_point_precise_t mouth[MOUTH_POINTS];
    mouth_points(mouth, center_x, center_y + 84.0f, mouth_width, mouth_height, phase);
    lv_point_precise_t mouth_center = {.x = center_x, .y = center_y + 84.0f};
    draw_polygon(layer, mouth, MOUTH_POINTS, mouth_center, s_eye_color, LV_OPA_COVER);

    /* A small state marker keeps the face readable without turning it into a dashboard. */
    lv_draw_arc_dsc_t arc;
    lv_draw_arc_dsc_init(&arc);
    arc.color = s_accent;
    arc.width = 3;
    arc.opa = LV_OPA_70;
    arc.center.x = (lv_coord_t)center_x;
    arc.center.y = (lv_coord_t)center_y;
    arc.radius = 207;
    arc.start_angle = 218;
    arc.end_angle = 218 + (uint16_t)(38U * ((uint32_t)mood + 1U));
    lv_draw_arc(layer, &arc);
}

static void motion_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    s_motion_tick++;
    lv_obj_invalidate(s_face);
}

static void touch_event_cb(lv_event_t *event)
{
    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) {
        return;
    }

    lv_point_t point;
    lv_indev_get_point(indev, &point);
    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED) {
        s_press_x = point.x;
        s_press_y = point.y;
    }
    else if (code == LV_EVENT_RELEASED && abs(point.x - s_press_x) < 32 &&
             abs(point.y - s_press_y) < 32) {
        s_motion_tick += 36U;
        lv_obj_invalidate(s_face);
    }
}

void grok_agent_ui_init(lv_display_t *display)
{
    palette_init();
    lv_obj_t *screen = lv_disp_get_scr_act(display);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, s_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    s_face = lv_obj_create(screen);
    lv_obj_remove_style_all(s_face);
    lv_obj_set_pos(s_face, 0, 0);
    lv_obj_set_size(s_face, UI_SCREEN_SIZE, UI_SCREEN_SIZE);
    lv_obj_add_flag(s_face, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_face, face_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
    lv_obj_add_event_cb(s_face, touch_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_face, touch_event_cb, LV_EVENT_RELEASED, NULL);

    lv_timer_create(motion_timer_cb, 33, NULL);
    lv_obj_invalidate(s_face);
}
