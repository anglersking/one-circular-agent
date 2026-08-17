# ONE Circular Agent: ESP-IDF example

这是 ONE 系列的第一个可直接编译和烧录的硬件例程。它运行在 ESP32-S3 上，驱动 OSPTEK AM173Q466466FLS 的 1.73 英寸 `466x466` CO5300 QSPI AMOLED 屏幕和 CST820 电容触摸，并加载一套 SquareLine Studio 导出的 LVGL 9 圆形界面。

这个例程是桌面圆形智能体的显示与交互底座。它不包含 Codex 使用量、家居控制或车辆协议；这些功能应由后续的桌面端 Agent Bridge 以 MQTT、HTTP 或 WebSocket 方式提供数据，避免把密钥和复杂登录态写入设备。

## 已验证环境

| 项目 | 实机验证值 |
| --- | --- |
| 主控 | ESP32-S3 QFN56 rev 0.2 |
| Flash / PSRAM | 16 MB Flash / 8 MB Octal PSRAM |
| 屏幕 | OSPTEK AM173Q466466FLS，CO5300，466x466 AMOLED |
| 触摸 | CST820（I2C） |
| ESP-IDF | 5.5.2 |
| LVGL | 9.4.0 |

已完成编译、烧录和串口启动检查：PSRAM、CO5300、CST820、LVGL 任务及 UI 初始化均成功。

## 目录说明

```text
example/idf/
├── main/                         # 板级初始化、GPIO 映射、LVGL 启动
├── components/
│   ├── agent_watch/              # SquareLine UI、图片、字体与 392 -> 466 适配器
│   └── esp_lcd_touch_cst820/     # CST820 ESP-IDF 触摸驱动
├── dependencies.lock             # 已验证的组件版本锁
├── sdkconfig.defaults*           # ESP32-S3 + Octal PSRAM 默认配置
└── partitions.csv                # 6 MB factory 应用分区
```

`managed_components/`、`build/` 和 `sdkconfig` 均由 ESP-IDF 在本机生成，因此不提交到仓库。

## 接线

本例程采用 OSPTEK 转接板的 ESP32-S3 参考接线。所有信号均为 3.3 V 电平，不能接 5 V。

### CO5300 显示 QSPI

| 屏幕信号 | ESP32-S3 GPIO | `main/main.c` 宏 |
| --- | ---: | --- |
| `QS_CLK` / SCK | 9 | `EXAMPLE_PIN_NUM_LCD_SCK` |
| `QS_IO0` / D0 | 10 | `EXAMPLE_PIN_NUM_LCD_D0` |
| `QS_IO1` / D1 | 11 | `EXAMPLE_PIN_NUM_LCD_D1` |
| `QS_IO2` / D2 | 12 | `EXAMPLE_PIN_NUM_LCD_D2` |
| `QS_IO3` / D3 | 13 | `EXAMPLE_PIN_NUM_LCD_D3` |
| `QS_CS` / CS | 14 | `EXAMPLE_PIN_NUM_LCD_CS` |
| `LCDRST` / RST | 15 | `EXAMPLE_PIN_NUM_LCD_RST` |
| `LCDTE` / TE | 16 | `EXAMPLE_PIN_NUM_LCD_TE`，本例程未使用 |

### CST820 触摸和供电

| 信号 | ESP32-S3 GPIO | 说明 |
| --- | ---: | --- |
| `TP_SCL` | 42 | I2C 时钟，400 kHz |
| `TP_SDA` | 41 | I2C 数据 |
| `TP_RST` | 40 | 触摸复位 |
| `TP_INT` | 39 | 触摸中断，低电平有效 |
| `3V3` | 3.3 V | 屏幕与触摸供电 |
| `GND` | GND | 必须与主控共地 |

QSPI 的 `D0~D3` 都是双向数据线，不能按普通 SPI 的 MOSI/MISO/DC 接法连接。首次点不亮时，先检查 3.3 V、共地、D0~D3 顺序、CS 和 RST；确认无误后再降低 `EXAMPLE_LCD_PIXEL_CLOCK_HZ`。

## 首次编译和烧录

1. 安装 ESP-IDF `5.5.x`，并打开已加载 ESP-IDF 环境的终端。
2. 进入本目录：

   ```bash
   cd example/idf
   ```

3. 首次构建时选择 ESP32-S3，然后编译。组件管理器会根据 `dependencies.lock` 下载已锁定版本。

   ```bash
   idf.py set-target esp32s3
   idf.py build
   ```

4. 找到串口并烧录。macOS 通常形如 `/dev/cu.usbmodemXXXX`，Linux 通常形如 `/dev/ttyACM0`，Windows 通常形如 `COM5`。

   ```bash
   idf.py -p /dev/cu.usbmodemXXXX flash monitor
   ```

串口出现下列日志即表示底层启动成功：

```text
ONE_CIRCULAR_AGENT: CO5300 QSPI 显示初始化完成 (466x466)
ONE_CIRCULAR_AGENT: CST820 触控初始化完成
ONE_CIRCULAR_AGENT: LVGL UI 创建完成，系统正常运行...
```

退出串口监视器使用 `Ctrl+]`。当 `sdkconfig` 或目标芯片配置混乱时，删除本目录下的 `sdkconfig` 和 `build/`，再从 `idf.py set-target esp32s3` 开始即可。

## UI 尺寸适配

SquareLine 原始工程的画布为 `392x392`，而目标 AMOLED 是 `466x466`。`components/agent_watch/ui_screen_adapter.c` 会将每个生成的屏幕放入 `392x392` 的 stage，再以相同倍率缩放到实际显示器；图片、字体、动画和触摸命中区会一起缩放，避免直接拉伸单个控件造成布局偏移。

UI 源码与 SquareLine 项目文件位于 `components/agent_watch/`。用 SquareLine Studio 修改 `Smartwatch.spj` 后，将导出目录重新设为该目录并导出 LVGL 9 代码；导出可能覆盖生成的 `ui.*`、`screens/`、`images/` 和 `fonts/` 文件。不要覆盖 `ui_screen_adapter.c`、`ui_screen_adapter.h` 和该目录的 `CMakeLists.txt`，它们负责 ESP-IDF 组件注册与 466x466 尺寸适配。

## 下一步

- 在 `main/main.c` 中按自己的开发板修改 GPIO、旋转、亮度和触摸镜像。
- 为 UI 加入一个只读状态模型，再从桌面端 Agent Bridge 接入 Codex 用量、Home Assistant/MQTT 或模拟车辆数据。
- 真实车辆场景第一阶段只读显示；不要将本例程用于制动、转向、动力或防盗等安全关键控制。
