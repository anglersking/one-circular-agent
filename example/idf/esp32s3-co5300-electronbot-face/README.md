# ONE Circular Agent: ElectronBot White-Eye Face

这是一个和 DeepSeek、Grok 完全隔离的 ESP-IDF 例程：在 OSPTEK `466x466` CO5300 QSPI AMOLED 上播放 ElectronBot 风格的**黑底白眼**表情。

- 黑色全屏背景，白色眼睛和白色嘴部。
- 表情资源来自 ElectronBot Standalone 的 `speak.json`，预渲染为 `466x466` GIF。
- 由 LVGL 9 内置 GIF 解码器在 ESP32-S3 上按帧播放，不运行 JavaScript，也不是单张图片。
- GIF 数据通过 `EMBED_FILES` 编译进固件，运行时从 Flash 读取，解码帧缓冲放在 LVGL 内存中。
- 触摸、Wi-Fi、API Key、DeepSeek 余额和图表都不在这个例程中。

## 来源和许可

本例程使用 [maker-community/ElectronBot.Standalone](https://github.com/maker-community/ElectronBot.Standalone) 中的 `src/ElectronBot.Standalone.Core/LottieFiles/speak.json`（MIT License）。仓库中的 `assets/electronbot_speak.json` 保留了原始 Lottie 文件，`assets/electronbot_speak.gif` 是由它预渲染并压成黑底后的设备资源。归属和许可证见 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。

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
ELECTRONBOT_FACE: ElectronBot white-eye animation ready (466x466, black background)
```

退出串口监视器按 `Ctrl+]`。烧录会覆盖板子当前运行的例程；切回 DeepSeek 或 Grok 时，进入对应目录重新执行同样的 `build`、`flash` 命令即可。

