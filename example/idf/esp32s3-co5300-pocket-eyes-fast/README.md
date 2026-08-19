# ONE Circular Agent: Pocket Fast Eyes

这是 Pocket 表情的第二个、完全独立的适配版本。它保留 Pocket 的夸张情绪，但把原始 `172×320` GIF 的黑色边框裁掉，只嵌入中间的眼睛区域，并将原始每帧约 `20 ms` 改为约 `12 ms`，让眨眼和表情变化更利落。

当前版本和 [`esp32s3-co5300-pocket-face/`](../esp32s3-co5300-pocket-face/) 并列存在：前者是完整 Pocket 画布基准版，本例程是“小眼睛、快动画”实验版，二者不共享 `main/`、组件、配置或构建目录。

## 效果和交互

- 黑色圆屏中只显示 Pocket 表情的眼睛主体，四周保留黑色留白。
- 普通表情资源为约 `96×208`，充电表情为约 `128×240`；运行时适度放大到圆屏中间，不铺满整张屏幕。
- 保留 `twece`、`anger`、`disdain`、`excited`、`once` 五个普通表情和 `charge` 特殊表情。
- 所有帧重编码为约 `12 ms`，比原始 20 ms 播放更快；表情形态本身来自 Pocket，愤怒、轻蔑、兴奋等对比会更明显。
- 轻点：切换下一个普通表情。
- 长按：进入 `charge`；在充电表情下再次长按返回普通表情。
- 10 秒无操作：暂停动画并把 AMOLED 降到低亮度；第一次触摸只唤醒，下一次触摸才切换。

## 硬件与引脚

ESP32-S3（16 MB Flash、8 MB Octal PSRAM）+ OSPTEK 1.73 英寸 `466×466` CO5300 QSPI AMOLED + CST820，逻辑电平 `3.3 V`。

| 信号 | GPIO |
| --- | ---: |
| QSPI SCK | 9 |
| QSPI D0 / D1 / D2 / D3 | 10 / 11 / 12 / 13 |
| LCD CS / RST | 14 / 15 |
| CST820 SCL / SDA | 42 / 41 |
| CST820 RST / INT | 40 / 39 |
| 3V3 / GND | 3.3 V / GND |

## 编译

使用 ESP-IDF `5.5.2`：

```bash
source /Users/yu/Downloads/xiaozhi-esp32/esp-idf/export.sh
cd example/idf/esp32s3-co5300-pocket-eyes-fast
idf.py set-target esp32s3
idf.py build
```

本工程使用与其他 CO5300 例程相同的 QSPI 初始化和 CST820 引脚。LVGL 显示缓冲使用内部 RAM 小行缓冲，PSRAM 留给 GIF 解码，避免 QSPI DMA 缓冲不足。

## 烧录和串口

```bash
ls /dev/cu.usbmodem*
idf.py -p /dev/cu.usbmodemXXXX flash monitor
```

正常启动日志类似：

```text
POCKET_EYES_FAST: Pocket fast-eyes ready: tap next, hold charge, 10s dim
POCKET_UI: tap: next exaggerated eye mood; hold: charge mode; touch wakes after dim
```

退出监视器按 `Ctrl+]`。烧录会覆盖板子当前运行的例程；切回完整 Pocket、DeepSeek 或其他例程时，进入对应目录重新执行自己的命令即可。

## 来源与许可

本例程的表情来源于 [ByCoCandy/Pocket](https://github.com/ByCoCandy/Pocket)，基于提交 [`a2ab6337bc2d4634e99a879f8dc78c2bea0ece20`](https://github.com/ByCoCandy/Pocket/commit/a2ab6337bc2d4634e99a879f8dc78c2bea0ece20)。本目录中的 GIF 是对上游 GIF 的**裁边和帧间隔调整派生资源**，不是新增的第三方表情。完整 MIT 文本见 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。
