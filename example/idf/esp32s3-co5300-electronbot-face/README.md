# ONE Circular Agent: ElectronBot White-Eye Face

这是一个和 DeepSeek、Grok 完全隔离的 ESP-IDF 例程：在 OSPTEK `466x466` CO5300 QSPI AMOLED 上绘制 ElectronBot 风格的**黑底白眼**表情。

- 黑色全屏背景，只绘制白色双眼。
- 不绘制嘴部，只保留白色双眼，避免嘴部在圆屏上显得突兀。
- 由 LVGL 9 内置 GIF 解码器在 ESP32-S3 上按帧播放，不运行 JavaScript，也不是单张图片。
- 保留 ElectronBot 原始动画的眨眼、睁闭眼和缓动效果；嘴部区域已逐帧遮为黑色，避免违和。
- 内置 8 个平滑眼睛状态：待机、惊讶、开心、害怕、生气、忙碌、失落、疑惑。
- **交互：轻点屏幕任意位置切换到下一个状态**；切换后该状态从第 1 帧重新播放。
- 触摸、Wi-Fi、API Key、DeepSeek 余额和图表都不在这个例程中。

## 来源和许可

本例程参考 [maker-community/ElectronBot.Standalone](https://github.com/maker-community/ElectronBot.Standalone) 中的 `src/ElectronBot.Standalone.Core/LottieFiles/speak.json`（MIT License）。`assets/electronbot_speak.gif` 和 `assets/electronbot_face_*.gif` 是原始动画经黑底化、遮嘴和眼睛形态变换后的设备资源；归属和许可证见 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。

这不是 ElectronBot 官方固件，也不包含官方硬件控制协议；这里只移植屏幕上的表情表现。

## 接线

硬件：ESP32-S3（16 MB Flash、8 MB Octal PSRAM）+ OSPTEK AM173Q466466FLS（CO5300）+ CST820，电平均为 `3.3 V`。

| 信号 | ESP32-S3 GPIO |
| --- | ---: |
| `QS_CLK` / SCK | 9 |
| `QS_IO0` / D0 | 10 |
| `QS_IO1` / D1 | 11 |
| `QS_IO2` / D2 | 12 |
| `QS_IO3` / D3 | 13 |
| `QS_CS` / CS | 14 |
| `LCDRST` / RST | 15 |
| `TP_SCL` / `TP_SDA` | 42 / 41 |
| `TP_RST` / `TP_INT` | 40 / 39 |
| `3V3` / `GND` | 3.3 V / GND |

## 编译和烧录

使用 ESP-IDF `5.5.2`：

```bash
cd example/idf/esp32s3-co5300-electronbot-face
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/cu.usbmodemXXXX flash monitor
```

串口启动后应看到：

```text
ELECTRONBOT_FACE: tap screen to cycle 8 smooth white-eye moods; no mouth
```

退出串口监视器按 `Ctrl+]`。烧录会覆盖板子当前运行的例程；切回 DeepSeek 或 Grok 时，进入对应目录重新执行同样的 `build`、`flash` 命令即可。
