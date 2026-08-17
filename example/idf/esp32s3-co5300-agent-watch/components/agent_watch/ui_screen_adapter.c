#include "ui_screen_adapter.h"

#define UI_SCALE_NORMAL 256

void ui_screen_adapter_apply(lv_obj_t * screen)
{
    if(screen == NULL || lv_obj_get_child_count(screen) == 0) {
        return;
    }

    lv_display_t * display = lv_obj_get_display(screen);
    if(display == NULL) {
        return;
    }

    const int32_t display_width = lv_display_get_horizontal_resolution(display);
    const int32_t display_height = lv_display_get_vertical_resolution(display);
    const int32_t scale_x = (display_width * UI_SCALE_NORMAL + UI_DESIGN_WIDTH / 2) / UI_DESIGN_WIDTH;
    const int32_t scale_y = (display_height * UI_SCALE_NORMAL + UI_DESIGN_HEIGHT / 2) / UI_DESIGN_HEIGHT;
    const int32_t scale = scale_x < scale_y ? scale_x : scale_y;

    lv_obj_t * stage = lv_obj_create(screen);
    lv_obj_remove_style_all(stage);
    lv_obj_set_size(stage, UI_DESIGN_WIDTH, UI_DESIGN_HEIGHT);
    lv_obj_set_align(stage, LV_ALIGN_CENTER);
    lv_obj_remove_flag(stage, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(stage, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_move_to_index(stage, 0);

    while(lv_obj_get_child_count(screen) > 1) {
        lv_obj_t * child = lv_obj_get_child(screen, 1);
        lv_obj_set_parent(child, stage);
    }

    lv_obj_update_layout(stage);

    if(scale != UI_SCALE_NORMAL) {
        lv_obj_set_style_transform_pivot_x(stage, UI_DESIGN_WIDTH / 2, LV_PART_MAIN);
        lv_obj_set_style_transform_pivot_y(stage, UI_DESIGN_HEIGHT / 2, LV_PART_MAIN);
        lv_obj_set_style_transform_scale_x(stage, scale, LV_PART_MAIN);
        lv_obj_set_style_transform_scale_y(stage, scale, LV_PART_MAIN);
    }
}
