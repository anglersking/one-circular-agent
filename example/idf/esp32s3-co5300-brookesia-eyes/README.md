# ONE Circular Agent: Espressif / Brookesia Eyes

这是一个与 Pocket、DeepSeek 和其他例程完全隔离的眼睛动画实验。它参考 Espressif `esp-brookesia` speaker 的做法：使用 `espressif2022/image_player` 播放预处理的 `.aaf` 动画帧，解码后直接以 RGB565 刷到 CO5300 面板的中间区域。

## 为什么这个版本更平滑

`pocket-eyes-fast` 仍然是低分辨率 GIF，再交给 LVGL 运行时放大，所以边缘会有锯齿。本例程不使用 `lv_gif_set_src()` 或运行时缩放：资源本身是 `284×126` 的最终画布，动画按 30 FPS 播放，四周保持黑色留白。这个布局对应 Espressif speaker 的 `284×126` 表情区域，适合 466×466 圆屏。

## 表情和操作

- 黑底、只显示两只眼睛，不放嘴，不铺满整个圆屏。
- 内置 happy、blink-fast、blink1、angry、dizzy、sad、sleep、blink-slow 八组动画。
- 30 FPS 播放；每一帧直接刷中间 `284×126` 区域。
- 轻触屏幕切换下一组表情。

本例程使用 Espressif speaker 发布的 `.aaf` 资源作为验证素材。后续可以替换成自己的蓝色眼睛素材，但必须先在电脑端预渲染到目标尺寸，再转换为 image_player 的 AAF/SBMP 格式，不能用小 GIF 直接放大。

## 硬件与 Pin 角

ESP32-S3（已验证 16 MB Flash、8 MB Octal PSRAM）+ OSPTEK 1.73 英寸 `466×466` CO5300 QSPI AMOLED + CST820，逻辑电平为 3.3 V。

| 信号 | GPIO |
| --- | ---: |
| QSPI SCK | 9 |
| QSPI D0 / D1 / D2 / D3 | 10 / 11 / 12 / 13 |
| LCD CS / RST | 14 / 15 |
| CST820 SCL / SDA | 42 / 41 |
| CST820 RST / INT | 40 / 39 |
| 3V3 / GND | 3.3 V / GND |

## 编译

每个例程都要在自己的目录中执行命令。先加载 ESP-IDF 环境：

```bash
source /Users/yu/Downloads/xiaozhi-esp32/esp-idf/export.sh
cd example/idf/esp32s3-co5300-brookesia-eyes
idf.py set-target esp32s3
idf.py build
```

第一次构建会由 Component Manager 下载 `espressif2022/image_player`。不需要把其他例程的 `managed_components`、`sdkconfig` 或 `build` 复制过来。

## 烧录和串口

```bash
ls /dev/cu.usbmodem*
idf.py -p /dev/cu.usbmodemXXXX flash monitor
```

把 `XXXX` 换成当前板子的实际串口。退出 monitor 使用 `Ctrl-]`。烧录会覆盖板子当前例程；要恢复 Pocket 或 DeepSeek，请进入对应目录重新编译和烧录。

## 来源和许可

动画资源来自 [Espressif esp-brookesia](https://github.com/espressif/esp-brookesia/tree/release/v0.6/products/speaker) 的 speaker emotion assets；播放器来自 [espressif2022/image_player](https://github.com/espressif2022/image_player)。它们分别按各自上游许可发布，详见 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。本例程的板级适配代码属于本仓库。
