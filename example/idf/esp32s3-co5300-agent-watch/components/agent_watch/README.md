# Agent Watch UI

SquareLine Studio export adapted for the One Circular Agent 466x466 AMOLED.

## Display sizing

- Original SquareLine design: 392x392, circular
- Original full-screen raster assets: 397x397
- Target panel: 466x466, circular
- Runtime layout scale: 466 / 392 = 1.1888
- LVGL transform scale at 466x466: 304 / 256

The SquareLine project remains at its native 392x392 design size. The
`ui_screen_adapter` moves each generated screen into a 392x392 stage, centers
it, and scales the complete stage to the active display. Images, fonts,
animations, widgets, and hit areas therefore keep the original proportions.

Configure the display driver to 466x466 before calling `ui_init()`:

```c
lv_display_set_resolution(display, 466, 466);
ui_init();
```

For the OSPTEK CO5300 example, keep the panel-specific values in the display
driver as well:

```c
#define LCD_WIDTH   466
#define LCD_HEIGHT  466
#define LCD_X_GAP   6
#define LCD_Y_GAP   0
```

## ESP32-S3 note

LVGL transforms a widget and its children through an intermediate layer. A
full-screen transformed UI needs additional draw memory, so use an ESP32-S3
with PSRAM and configure LVGL allocation accordingly. For a final production
build, recreating the SquareLine project natively at 466x466 will reduce the
runtime transform cost. The adapter is intended to make the current 392x392
export usable immediately without manually changing hundreds of coordinates.

## Regenerating from SquareLine

SquareLine regeneration can overwrite `ui.c`, `ui.h`, `ui_helpers.c`, and
`CMakeLists.txt`. After exporting again, restore these integration points:

1. Add `ui_screen_adapter.c` to `SOURCES`.
2. Include `ui_screen_adapter.h` from `ui.h`.
3. Apply the adapter to each initialized screen in `ui_init()`.
4. Apply the adapter after lazy screen creation in `_ui_screen_change()`.
