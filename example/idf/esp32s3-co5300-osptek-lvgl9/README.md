# OSPTEK CO5300 LVGL 9 原厂基准例程

这是 OSPTEK 1.73 英寸 `466x466` CO5300 QSPI AMOLED 的原厂 ESP32-S3 + ESP-IDF + LVGL 9 例程。它初始化 CO5300 显示、CST820 触摸和 `esp_lvgl_adapter`，然后显示 LVGL 官方 `lv_demo_widgets()` 控件演示。

此工程用于确认“屏幕亮、颜色正常、触摸可用”。不要在它中直接开发 ONE UI；桌面圆形智能体请使用相邻的 [`esp32s3-co5300-agent-watch/`](../esp32s3-co5300-agent-watch/README.md) 工程。两个工程完全独立，可以分别构建和烧录。

## 上游来源与保留范围

- 上游资料仓库：[OSPTEK Gitee](https://gitee.com/osptek/1.73-amoled-466x466-qspi-co5300)
- 上游工程路径：`versions/AM173Q466466FLS/examples/esp32s3-idf5_co5300-qspi_esp-lvgl-adapter_lvgl9/`
- 导入版本：上游提交 `e5f956c2f053604f6f30a8a8c27605f940083999`（2026-08-14）

除本 README 和 `dependencies.lock` 中的 ESP-IDF 版本外，工程源代码、默认配置和初始化命令均保持上游版本。为匹配本仓库已验证的 ESP-IDF `5.5.2`，锁文件的 `idf` 条目从上游的 `5.5.3` 调整为 `5.5.2`；这不改变显示或触摸源码。`sdkconfig`、`managed_components/` 和 `build/` 是本机生成文件，因此不导入 Git；首次构建时由 ESP-IDF 生成。

## 硬件与接线

适用硬件：ESP32-S3（16 MB Flash、8 MB Octal PSRAM）+ OSPTEK AM173Q466466FLS（CO5300）+ CST820 触摸。所有供电和信号均为 `3.3 V`，不得接 `5 V`。

| 信号 | ESP32-S3 GPIO |
| --- | ---: |
| `QS_CLK` / SCK | 9 |
| `QS_IO0` / D0 | 10 |
| `QS_IO1` / D1 | 11 |
| `QS_IO2` / D2 | 12 |
| `QS_IO3` / D3 | 13 |
| `QS_CS` / CS | 14 |
| `LCDRST` / RST | 15 |
| `LCDTE` / TE | 16（本例程未使用） |
| `TP_SCL` | 42 |
| `TP_SDA` | 41 |
| `TP_RST` | 40 |
| `TP_INT` | 39 |
| `3V3` / `GND` | 3.3 V / GND |

GPIO 定义位于 [`main/main.c`](main/main.c)。若换用不同的 ESP32-S3 开发板，只调整该文件中的 GPIO 宏；QSPI 的 `D0~D3` 是四根双向数据线，不能按普通 SPI 的 `MOSI/MISO/DC` 接法连接。

## 编译与烧录

1. 安装 ESP-IDF `5.5.2` 并在已加载 ESP-IDF 环境的终端执行以下命令。
2. 从仓库根目录进入本工程：

   ```bash
   cd example/idf/esp32s3-co5300-osptek-lvgl9
   idf.py set-target esp32s3
   idf.py build
   ```

3. 接上开发板后烧录并打开串口监视器。macOS 通常使用 `/dev/cu.usbmodemXXXX`，Linux 通常使用 `/dev/ttyACM0`，Windows 使用 `COM5` 一类的串口号。

   ```bash
   idf.py -p /dev/cu.usbmodemXXXX flash monitor
   ```

启动成功时，串口会依次出现 CO5300、CST820、`esp_lvgl_adapter` 和 `LVGL UI` 初始化成功的日志，屏幕显示 LVGL widgets 演示。退出串口监视器使用 `Ctrl+]`。

烧录会覆盖板子当前程序。之后要切回 ONE UI，只需进入 `../esp32s3-co5300-agent-watch/`，重新执行该例程 README 中的构建和烧录命令。

## 常见排查

- 屏幕不亮：先检查 `3V3`、GND、QSPI `D0~D3` 顺序、CS 和 RST。
- 画面不稳定：先把 `main/main.c` 的 `EXAMPLE_LCD_PIXEL_CLOCK_HZ` 从 40 MHz 降低，再检查线材和供电。
- 构建配置错乱：删除本目录生成的 `build/` 和 `sdkconfig`，然后重新执行 `idf.py set-target esp32s3`。
