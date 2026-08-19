# ONE Circular Agent: Pocket Face

这是一个完全独立的 ESP-IDF 例程，把 [ByCoCandy/Pocket](https://github.com/ByCoCandy/Pocket) 的 GIF 表情体验移植到 ONE 圆形智能体硬件：ESP32-S3 + OSPTEK 1.73 英寸 `466×466` CO5300 QSPI AMOLED + CST820 触摸屏。

它不会引用或修改 DeepSeek、ElectronBot、Grok 等例程的 `main/`、组件、配置或构建目录。切换例程时只需进入对应目录重新编译、烧录。

## 已移植的体验

- 内置上游 5 个普通表情：`twece`、`anger`、`disdain`、`excited`、`once`。
- 内置特殊 `charge` 充电表情。
- 开机随机选择一个普通表情；GIF 播完后自动随机切换，并避免连续重复同一个表情。
- 轻点圆屏：按顺序切换到下一个普通表情。
- 长按圆屏：进入充电表情；再次长按返回随机普通表情。
- 10 秒无操作后暂停动画并降低 AMOLED 亮度；触摸一次恢复亮度和动画，该次触摸不会误切表情。
- 原始资源是 `172×320` 竖屏 GIF，运行时等比缩放为约 `251×466` 并在圆屏黑底中居中，不拉伸人物比例。

上游的 IMU 晃动唤醒和 GPIO 充电检测没有硬套到本板：当前屏幕板没有确认安装 LSM6DS3TRC，且 GPIO0 也不是已经确认的充电状态脚。第一版用 CST820 的轻点、长按和触摸唤醒替代；以后外接 IMU 或充电检测电路时，可以在这个独立例程中继续增加，不影响其他工程。

## 硬件与接线

目标硬件：ESP32-S3（16 MB Flash、8 MB Octal PSRAM）+ AM173Q466466FLS / CO5300 + CST820。逻辑电平为 `3.3 V`。

| 屏幕/触摸信号 | ESP32-S3 GPIO |
| --- | ---: |
| `QS_CLK` / QSPI SCK | 9 |
| `QS_IO0` / D0 | 10 |
| `QS_IO1` / D1 | 11 |
| `QS_IO2` / D2 | 12 |
| `QS_IO3` / D3 | 13 |
| `QS_CS` / CS | 14 |
| `LCDRST` / LCD RST | 15 |
| `TP_SCL` | 42 |
| `TP_SDA` | 41 |
| `TP_RST` | 40 |
| `TP_INT` | 39 |
| `3V3` / `GND` | 3.3 V / GND |

不要把原 Pocket 的 ST7789 引脚照搬到这块板。上游使用 ESP32-C3 + SPI ST7789，而本例程使用 ESP32-S3 + QSPI CO5300，两者的屏幕接口和引脚定义不同。

## 目录隔离

```text
esp32s3-co5300-pocket-face/
├── assets/                         # 6 个上游 Pocket GIF
├── components/esp_lcd_touch_cst820/ # 本例程自己的触摸驱动
├── main/                           # CO5300 初始化和 Pocket UI
├── CMakeLists.txt
├── partitions.csv                 # 6 MB app 分区，容纳嵌入式 GIF
├── sdkconfig.defaults
└── sdkconfig.defaults.esp32s3
```

`build/`、`managed_components/` 和 `sdkconfig` 都是本机生成文件，已经由仓库 `.gitignore` 忽略，不应提交。

## 编译

已按 ESP-IDF `5.5.2` 配置。先加载 ESP-IDF 环境，再进入本例程目录：

```bash
source /Users/yu/Downloads/xiaozhi-esp32/esp-idf/export.sh
cd example/idf/esp32s3-co5300-pocket-face
idf.py set-target esp32s3
idf.py build
```

工程使用 16 MB QIO Flash、8 MB Octal PSRAM。显示刷新采用内部 RAM 小行缓冲，PSRAM 留给 GIF 解码和 LVGL 任务，避免 CO5300 QSPI 刷屏时因 DMA bounce buffer 申请失败而报 `ESP_ERR_NO_MEM`。

## 烧录与串口

先用以下命令查看串口：

```bash
ls /dev/cu.usbmodem*
```

将命令中的端口替换为实际设备，例如：

```bash
idf.py -p /dev/cu.usbmodem5B5F0058581 flash monitor
```

只烧录、不打开监视器：

```bash
idf.py -p /dev/cu.usbmodem5B5F0058581 flash
```

退出串口监视器按 `Ctrl+]`。正常启动会看到类似日志：

```text
POCKET_FACE: Pocket face ready: tap next, hold charge, 10s dim
POCKET_UI: tap: next face; hold: charge mode; touch wakes after dim
```

烧录会覆盖板子当前运行的例程，但不会删除仓库里的其他例程；想切回 DeepSeek 时，进入 DeepSeek 例程目录重新执行它自己的 `idf.py build` 和 `flash` 即可。

## 来源与许可

Pocket 表情资源和交互设计来自 [ByCoCandy/Pocket](https://github.com/ByCoCandy/Pocket)，移植基于提交 [`a2ab6337bc2d4634e99a879f8dc78c2bea0ece20`](https://github.com/ByCoCandy/Pocket/commit/a2ab6337bc2d4634e99a879f8dc78c2bea0ece20)。原项目为 MIT License，版权和完整许可文本见 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。

本例程是面向 ONE Circular Agent 硬件的非官方 ESP-IDF 移植，不是 ByCoCandy/Pocket 原硬件固件。
