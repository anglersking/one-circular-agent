# ONE Circular Agent: DeepSeek Feature Test

这是 DeepSeek 余额例程的实验隔离目录。它复制了可运行的 Wi-Fi、余额查询、历史柱状图和圆屏触摸基础，后续只在这里试验 ElectronBot 风格的**蓝色眼睛动画**。

`esp32s3-co5300-deepseek-quota` 是稳定基线；本目录是 feature test。两个工程各自有独立的 `CMakeLists.txt`、`main/`、`components/`、`sdkconfig` 和 `build/`，不会互相覆盖。

当前版本已经把首页脸部替换为蓝色 ElectronBot 风格 GIF 动画；余额页、Wi-Fi 和 API 数据链路仍沿用基线。Wi-Fi 在连续失败后每 30 秒继续尝试重连，恢复网络后会自动回到余额同步。

模型字段目前用于屏幕身份显示。DeepSeek `/user/balance` 不接收模型参数，因此改 `AGENT_MODEL_NAME` 不会改变余额结果；后续会把 provider、model 和 usage/balance endpoint 拆成适配器，接入其他模型时只增加对应适配器。

这是 ONE 系列的独立 ESP-IDF 例程：在 OSPTEK 1.73 inch `466x466` CO5300 QSPI AMOLED 上显示一个像素风 Agent 表情，并通过 Wi-Fi 查询 DeepSeek 账户余额。

- 首页：黑底蓝眼 ElectronBot 风格动画，保留外圈仪表环，不再显示顶部五个装饰方块或嘴部图形；同时显示 Wi-Fi / API 同步状态。
- 向左或向右划：切到余额页，显示可用余额、赠送余额、充值余额和最近 8 次同步的柱状图。
- 主题：默认蓝白；配置 `AGENT_THEME_AIRPORT 1` 切换到黄灰机场仪表风。
- 轮询：默认每 5 分钟查询一次。历史数据保存在设备 NVS 中，重启后仍可显示。

它与 `esp32s3-co5300-osptek-lvgl9` 原厂硬件基准和 `esp32s3-co5300-agent-watch` UI 例程完全隔离。

## 配置文件

此版本不使用串口配置。请只编辑本机的 `main/agent_config.h`，该文件已被 Git 忽略，不能提交或粘贴到公开 Issue。

本地工作目录已经创建了这个文件。其他开发者新克隆仓库时，先执行：

```bash
cd example/idf/esp32s3-co5300-deepseek-featuretest
cp main/agent_config.h.example main/agent_config.h
```

然后编辑 `main/agent_config.h`：

```c
#define AGENT_WIFI_SSID "your-wifi-name"
#define AGENT_WIFI_PASSWORD "your-wifi-password"
#define AGENT_API_KEY "your-deepseek-api-key"
#define AGENT_BALANCE_URL "https://api.deepseek.com/user/balance"
#define AGENT_MODEL_NAME "deepseek-chat"
#define AGENT_DISPLAY_NAME "DEEPSEEK AGENT"
#define AGENT_THEME_AIRPORT 0
#define AGENT_POLL_INTERVAL_SECONDS 300
```

需要你自己填写的只有 Wi-Fi 名称、Wi-Fi 密码和 DeepSeek API Key。URL 保持默认即可；`AGENT_MODEL_NAME` 目前只作为屏幕身份标识，不会调用模型推理接口。

DeepSeek 的 `/user/balance` 接口返回当前账户货币余额、赠送余额和充值余额，并不提供通用的 token 用量历史。因此此例程的柱状图记录设备实际查询到的最近 8 次余额，用于观察余额变化。以后接入 Codex、其他模型或自建 Agent Bridge 时，可以保留 UI，替换 `deepseek_fetch_balance()` 的数据源即可。

蓝色表情资源来自 [maker-community/ElectronBot.Standalone](https://github.com/maker-community/ElectronBot.Standalone) 的 Lottie 表情，已在本地转换为黑底蓝眼 GIF；归属和许可证见 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。

API Key 会被编译进固件；只给测试设备使用，泄露、转让设备或开源前请在 DeepSeek 平台撤销该 Key。后续会另加串口 / 配网页配置，不需要重新编译固件。

## 接线

适用硬件：ESP32-S3（16 MB Flash、8 MB Octal PSRAM）+ OSPTEK AM173Q466466FLS（CO5300）+ CST820。所有信号使用 `3.3 V`。

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

使用 ESP-IDF `5.5.2`，在加载了 ESP-IDF 环境的终端中执行：

```bash
cd example/idf/esp32s3-co5300-deepseek-featuretest
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/cu.usbmodemXXXX flash monitor
```

烧录会覆盖板子当前程序。启动后先看到 Agent 表情页；Wi-Fi 成功联网且 API Key 有效时，状态会变为 `BALANCE READY`。横向划动屏幕可进入余额页。

## 排查

- 一直显示 `CONFIG REQUIRED`：`main/agent_config.h` 里的 Wi-Fi SSID 仍为空，或这个文件没有创建。
- 显示 `WIFI OFFLINE`：检查 SSID 和密码，确认网络允许 2.4 GHz ESP32-S3 接入。
- 显示 `SYNC FAILED`：检查 API Key、账号余额权限和 HTTPS URL。串口日志不会打印 API Key。
- 屏幕不亮或触摸错误：先烧录相邻的 [`esp32s3-co5300-osptek-lvgl9`](../esp32s3-co5300-osptek-lvgl9/README.md) 原厂基准例程验证硬件。
