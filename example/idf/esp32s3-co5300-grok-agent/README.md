# ONE Circular Agent: Grok-Style Icon Study

这是一个独立 ESP-IDF 例程，面向 OSPTEK 1.73 inch `466x466` CO5300 QSPI AMOLED。它保留 DeepSeek 余额查询与本地柱状历史，并将首页换成适合圆屏的黑色 blob Agent 表情。

- 首页：纸白/黑墨风格的圆润 Agent，眼睛会在 `CURIOUS`、`HAPPY`、`PLAYFUL`、`THINKING`、`LISTENING` 和 `IDLE` 之间轮换，并在 idle 时眨眼。
- 横向划屏：进入 DeepSeek 余额、赠送余额、充值余额和最近 8 次同步柱状图。
- 主题：默认纸白/黑墨；设置 `AGENT_THEME_AIRPORT 1` 切到深灰/黄灰机场风。
- 数据：与 `esp32s3-co5300-deepseek-quota` 使用相同的 HTTPS DeepSeek `/user/balance` 查询方式；余额历史保存在 NVS。

它与其他例程完全隔离。烧录这个目录只会覆盖板子上的当前固件，不会改动 `esp32s3-co5300-deepseek-quota` 或原厂基准例程的代码。

## 关于参考项目

设计研究参考了 [blessonism/grok-icon-study](https://github.com/blessonism/grok-icon-study) 的状态机思路和圆润眼睛的视觉方向。该上游仓库明确标注：其抽取的 Grok Bot 素材、几何数据和商标仅供学习，不能商用或再分发。

因此本例程**没有复制**上游 JavaScript、SVG、位图、`geometry-data.js` 或 xAI 的多边形眼睛数据。`main/grok_agent_ui.c` 只用 LVGL 基础圆角对象实现了一个独立的桌面 Agent 表情。它不是 xAI/Grok 官方软件或商标授权实现；若要使用官方素材，应先取得权利人的书面授权。

## 配置

不使用串口配置。先复制模板，配置只保留在本机：

```bash
cd example/idf/esp32s3-co5300-grok-agent
cp main/agent_config.h.example main/agent_config.h
```

编辑 `main/agent_config.h`：

```c
#define AGENT_WIFI_SSID "your-wifi-name"
#define AGENT_WIFI_PASSWORD "your-wifi-password"
#define AGENT_API_KEY "your-deepseek-api-key"
#define AGENT_BALANCE_URL "https://api.deepseek.com/user/balance"
#define AGENT_MODEL_NAME "deepseek-chat"
#define AGENT_DISPLAY_NAME "ONE AGENT"
#define AGENT_THEME_AIRPORT 0
#define AGENT_POLL_INTERVAL_SECONDS 300
```

`agent_config.h` 已被 Git 忽略，不能提交、截图或粘贴到公开 Issue。API Key 会进入固件，只能用于自己的测试设备；转让设备或公开前应撤销该 Key。

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

使用 ESP-IDF `5.5.2`。在已加载 ESP-IDF 环境的终端中：

```bash
cd example/idf/esp32s3-co5300-grok-agent
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/cu.usbmodemXXXX flash monitor
```

实机上使用过的串口格式是 `/dev/cu.usbmodemXXXX`；先用 `ls /dev/cu.usbmodem*` 确认实际端口。退出监视器按 `Ctrl+]`，这样串口不会一直被占用。

启动后首页会先显示表情和联网状态；联网成功后日志出现 `Balance synced: <amount> CNY`。向左划进入余额图表，向右划回到表情页。

## 已验证

- ESP-IDF `5.5.2`
- LVGL `9.4.0`
- ESP32-S3（16 MB Flash、8 MB PSRAM）
- CO5300 QSPI + CST820

固件构建产物为 `build/one_circular_grok_agent.bin`。本例程构建时约占 1.31 MiB，适合 6 MiB 工厂应用分区。

## 排查

- 显示 `CONFIG REQUIRED`：创建并填写 `main/agent_config.h`。
- 显示 `WIFI OFFLINE`：检查 2.4 GHz Wi-Fi、SSID 与密码。
- 显示 `SYNC FAILED`：检查 DeepSeek Key 和余额接口权限；串口不会打印 Key。
- 屏幕不亮或触摸异常：先烧录 [`esp32s3-co5300-osptek-lvgl9`](../esp32s3-co5300-osptek-lvgl9/README.md) 验证硬件。
