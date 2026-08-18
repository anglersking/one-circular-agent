# ONE Circular Agent: Grok-Style Icon Study

这是一个**纯显示与触摸**的独立 ESP-IDF 例程，面向 OSPTEK 1.73 inch `466x466` CO5300 QSPI AMOLED。

- 纸白/黑墨风格的圆润 blob Agent。
- `CURIOUS`、`HAPPY`、`PLAYFUL`、`THINKING`、`LISTENING` 和 `IDLE` 六种眼睛状态。
- 自动轮换、呼吸浮动和 idle 眨眼；点击屏幕立即切换下一种状态。
- 不连接 Wi-Fi，不读取 DeepSeek，不需要 API Key、URL 或配置文件，也没有余额与图表页。

DeepSeek 余额功能只存在于相邻的 [`esp32s3-co5300-deepseek-quota`](../esp32s3-co5300-deepseek-quota/README.md) 例程。两个工程拥有各自的 `main/`、构建配置、依赖锁和固件，互不引用。

## 关于参考项目

设计研究参考了 [blessonism/grok-icon-study](https://github.com/blessonism/grok-icon-study) 的状态机思路和圆润眼睛的视觉方向。该上游仓库明确标注：其抽取的 Grok Bot 素材、几何数据和商标仅供学习，不能商用或再分发。

因此本例程**没有复制**上游 JavaScript、SVG、位图、`geometry-data.js` 或 xAI 的多边形眼睛数据。`main/grok_agent_ui.c` 只用 LVGL 基础圆角对象独立实现表情。它不是 xAI/Grok 官方软件或商标授权实现。

## 工程边界

```text
esp32s3-co5300-grok-agent/
├── main/main.c             # CO5300、CST820、LVGL 初始化
├── main/grok_agent_ui.c    # 纯本地表情与点击交互
├── components/             # 本例程自己的 CST820 驱动
├── sdkconfig.defaults*     # 本例程自己的 ESP-IDF 配置
└── README.md
```

这个目录中没有 `agent_config.h`，源码也不依赖 `esp_wifi`、`esp_http_client`、`json` 或 `nvs_flash`。

## 接线

硬件：ESP32-S3（16 MB Flash、8 MB Octal PSRAM）+ OSPTEK AM173Q466466FLS（CO5300）+ CST820。所有电平都是 `3.3 V`。

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

## 编译与烧录

使用 ESP-IDF `5.5.2`：

```bash
cd example/idf/esp32s3-co5300-grok-agent
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/cu.usbmodemXXXX flash monitor
```

退出串口监视器按 `Ctrl+]`。启动成功后串口会显示：

```text
GROK_ICON: Starting pure LVGL icon study
GROK_ICON: Pure icon demo ready: no Wi-Fi, API key, balance or chart
```

## 已验证

- ESP-IDF `5.5.2`
- LVGL `9.4.0`
- ESP32-S3（16 MB Flash、8 MB PSRAM）
- CO5300 QSPI + CST820

屏幕不亮或触摸异常时，先烧录 [`esp32s3-co5300-osptek-lvgl9`](../esp32s3-co5300-osptek-lvgl9/README.md) 验证硬件。
