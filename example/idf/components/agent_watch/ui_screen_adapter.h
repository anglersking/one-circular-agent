#ifndef UI_SCREEN_ADAPTER_H
#define UI_SCREEN_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#define UI_DESIGN_WIDTH 392
#define UI_DESIGN_HEIGHT 392

/**
 * Move a generated SquareLine screen into a 392x392 design stage and scale
 * the stage to the active display while preserving the circular aspect ratio.
 */
void ui_screen_adapter_apply(lv_obj_t * screen);

#ifdef __cplusplus
}
#endif

#endif
