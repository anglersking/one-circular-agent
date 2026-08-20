# ONE Circular Agent: DeepSeek Feature Test

这是 DeepSeek 余额例程的实验隔离目录。它复制了可运行的 Wi-Fi、余额查询、历史柱状图和圆屏触摸基础，并在这里试验 Brookesia 风格的**蓝色眼睛动画**。

`esp32s3-co5300-deepseek-quota` 是稳定基线；本目录是 feature test。两个工程各自有独立的 `CMakeLists.txt`、`main/`、`components/`、`sdkconfig` 和 `build/`，不会互相覆盖。

当前版本已经把首页脸部替换为 Brookesia AAF 表情动画，并在 LVGL Canvas 中映射为 DeepSeek 蓝色；愤怒表情遵循原始动画时间线，先显示蓝色圆眼，再随愤怒造型渐变成红色，结束时恢复蓝色。余额页、Wi-Fi 和 API 数据链路仍沿用基线。Wi-Fi 在连续失败后每 30 秒继续尝试重连，恢复网络后会自动回到余额同步。联网后还会通过 NTP 显示本地时间，并提供周五状态页、日历页、天气/网络页和设置状态页。

模型字段目前用于屏幕身份显示。DeepSeek `/user/balance` 不接收模型参数，因此改 `AGENT_MODEL_NAME` 不会改变余额结果；后续会把 provider、model 和 usage/balance endpoint 拆成适配器，接入其他模型时只增加对应适配器。

这是 ONE 系列的独立 ESP-IDF 例程：在 OSPTEK 1.73 inch `466x466` CO5300 QSPI AMOLED 上显示一个像素风 Agent 表情，并通过 Wi-Fi 查询 DeepSeek 账户余额。

- 首页：显示带抗锯齿边缘的蓝色 AAF 动画眼睛，动画底色与表盘背景融合，不显示黑色矩形、顶部五个装饰方块或嘴部图形；同时保留外圈仪表环和 Wi-Fi / API 同步状态。
- 表情：内置开心、眨眼、快速眨眼、慢速眨眼、愤怒、晕眩、难过和睡眠。愤怒动画由蓝眼渐变为红眼，其他表情保持蓝色。
- 向左或向右划：切到余额页，显示可用余额、赠送余额、充值余额和最近 8 次同步的柱状图。
- 继续横向滑动：进入 `FRIDAY CHECK` 页，周五显示绿色圆环，其他日期显示红色圆环；网络时间未同步时显示黄色状态。
- 再滑动：进入 `CALENDAR` 页显示公历和星期；农历/黄历卡片已预留独立数据源位置，暂不伪造内容。
- 再滑动：进入 `WEATHER` 页，使用 Open-Meteo 查询天气，同时显示网络 IP 和 RSSI。
- 最后进入 `SETTINGS` 页，显示 Provider、Model、Wi-Fi 和 API Key 是否已配置；敏感输入仍使用本地配置文件，后续再接手机网页配置。`FACE MOODS` 设为 `RANDOM` 时每 5 秒随机更换表情，设为 `HOLD` 时可在主页长按或上滑切换表情。`AUTO PAGES` 单独控制页面轮播，点击 `2S`、`5S` 或 `10S` 选择轮播间隔；关闭时仍使用手动左右滑动。
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
#define AGENT_PROVIDER_NAME "DeepSeek"
#define AGENT_MODEL_NAME "deepseek-chat"
#define AGENT_DISPLAY_NAME "DEEPSEEK AGENT"
#define AGENT_TIMEZONE "CST-8"
#define AGENT_LOCATION_NAME "SHANGHAI"
#define AGENT_WEATHER_URL "https://api.open-meteo.com/v1/forecast?latitude=31.2304&longitude=121.4737&current=temperature_2m,weather_code,wind_speed_10m&timezone=auto"
#define AGENT_THEME_AIRPORT 0
#define AGENT_POLL_INTERVAL_SECONDS 300
```

需要你自己填写的只有 Wi-Fi 名称、Wi-Fi 密码和 DeepSeek API Key。URL 保持默认即可；`AGENT_MODEL_NAME` 目前只作为屏幕身份标识，不会调用模型推理接口。

DeepSeek 的 `/user/balance` 接口返回当前账户货币余额、赠送余额和充值余额，并不提供通用的 token 用量历史。因此此例程的柱状图记录设备实际查询到的最近 8 次余额，用于观察余额变化。以后接入 Codex、其他模型或自建 Agent Bridge 时，可以保留 UI，替换 `deepseek_fetch_balance()` 的数据源即可。

当前 AAF 表情资源来自 [Espressif esp-brookesia speaker](https://github.com/espressif/esp-brookesia/tree/release/v0.6/products/speaker)，通过 `espressif2022/image_player` 解码后映射成产品配色；归属和许可证见 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。旧的 ElectronBot GIF 仍保留作对照资源，但当前主页不再加载它。

## 自己制作和加入表情

这套表情不是一张铺满 `466×466` 圆屏的图片，而是一块独立的 `284×126` 眼睛动画。这样文件更小，也方便把同一套眼睛放到不同的表盘、机器人头部或胸部 UI 中。

最简单有两种做法：

1. **替换已有表情**：生成同名 AAF，覆盖 `assets/` 中对应文件，不需要改 C 代码。
2. **增加新表情**：生成新的 AAF，再把它注册到 `CMakeLists.txt` 和 `brookesia_face_player.c`。

### 1. 制作 GIF 源文件

可以使用 Aseprite、Photoshop、After Effects、Blender、Figma 插件或任意 GIF 工具。推荐规格：

| 项目 | 建议值 |
| --- | --- |
| 画布 | `284×126 px`，必须与当前 Canvas 一致 |
| 背景 | 纯黑 `#000000` |
| 眼睛 | 白色或灰度，代码会在运行时重新着色 |
| 帧率 | `30 FPS`，约每帧 `33 ms` |
| 循环 | 开启 |
| 单段时长 | 建议 `0.5–4 秒` |

注意：黑色会被当成透明背景，灰度亮度会被当成眼睛的不透明度。边缘使用灰色抗锯齿可以更平滑；背景如果不是纯黑，圆屏上可能再次出现矩形底色。不要直接在 GIF 里画蓝色或红色，当前播放器主要读取亮度并在运行时着色。

如果只是修改现有表情，建议先用同样尺寸制作，例如：

```text
my_happy.gif
```

文件名只使用小写英文字母、数字和下划线，不要使用空格、中文或连字符，后面生成 C 链接符号会更省事。

### 2. 安装 AAF 转换工具依赖

先进入本例程并加载 ESP-IDF。第一次使用时运行 `reconfigure`，它会下载 `image_player` 组件及转换脚本：

```bash
cd example/idf/esp32s3-co5300-deepseek-featuretest
source /path/to/esp-idf/export.sh
idf.py reconfigure
```

转换脚本需要 Pillow、NumPy 和 scikit-learn。建议单独建立 Python 虚拟环境，不污染 ESP-IDF 的 Python 环境：

```bash
ONE_AAF_VENV="/tmp/one-aaf-venv"
python3 -m venv "$ONE_AAF_VENV"
source "$ONE_AAF_VENV/bin/activate"
python -m pip install --upgrade pip
python -m pip install pillow numpy scikit-learn
```

虚拟环境放在系统临时目录，不会进入 Git；用完可执行 `deactivate` 退出。

### 3. 把 GIF 转成 AAF

每次最好只转换一个 GIF，并使用空的输入、输出目录：

```bash
ONE_FACE_INPUT_DIR="$(mktemp -d /tmp/one-face-input.XXXXXX)"
ONE_FACE_OUTPUT_DIR="$(mktemp -d /tmp/one-face-output.XXXXXX)"
cp /path/to/my_happy.gif "$ONE_FACE_INPUT_DIR/"

python managed_components/espressif2022__image_player/script/gif_to_aaf.py \
  "$ONE_FACE_INPUT_DIR" \
  "$ONE_FACE_OUTPUT_DIR" \
  --split 16 \
  --depth 4 \
  --enable-huffman

echo "$ONE_FACE_OUTPUT_DIR"
```

成功后会在刚刚输出的临时目录中得到：

```text
my_happy.aaf
```

参数含义：

- `--split 16`：把一帧按 16 行分块解码，适合 ESP32-S3 内存和刷新方式。
- `--depth 4`：4 位灰度，文件较小，做黑底单色眼睛通常已经足够。
- `--depth 8`：灰度更细腻但文件更大，只在渐变边缘确实需要时使用。
- `--enable-huffman`：进一步压缩资源，建议开启。

### 4A. 最简单：替换已有表情

比如要替换 `happy`，将生成的文件覆盖为工程使用的名字：

```bash
cp "$ONE_FACE_OUTPUT_DIR/my_happy.aaf" \
  assets/emotion_happy_284_126.aaf
idf.py build
idf.py -p /dev/cu.usbmodemXXXX flash
```

已有名称包括：

```text
emotion_happy_284_126.aaf
emotion_blink_fast_284_126.aaf
emotion_blink1_284_126.aaf
emotion_angry_284_126.aaf
emotion_dizzy_284_126.aaf
emotion_sad_284_126.aaf
emotion_sleep_284_126.aaf
emotion_blink_slow_284_126.aaf
```

覆盖同名文件后建议执行一次完整 `idf.py build`，不要只复制文件后直接使用旧的 BIN。

### 4B. 增加一个全新表情

假设新资源叫 `emotion_surprised_284_126.aaf`，先复制到资源目录：

```bash
cp "$ONE_FACE_OUTPUT_DIR/my_surprised.aaf" \
  assets/emotion_surprised_284_126.aaf
```

然后完成三处注册。

第一处，在 `main/CMakeLists.txt` 的 `EMBED_FILES` 中加入：

```cmake
"../assets/emotion_surprised_284_126.aaf"
```

第二处，在 `main/brookesia_face_player.c` 中声明链接器生成的资源符号：

```c
extern const uint8_t _binary_emotion_surprised_284_126_aaf_start[];
extern const uint8_t _binary_emotion_surprised_284_126_aaf_end[];
```

第三处，把 `EYE_COUNT` 加一，并在 `s_emotions[]` 中加入表情。最后三个数字是目标 RGB 颜色，最后一个字段表示是否使用愤怒专用的蓝红时间线：

```c
{
    _binary_emotion_surprised_284_126_aaf_start,
    _binary_emotion_surprised_284_126_aaf_end,
    "surprised",
    23, 105, 255,
    false,
},
```

表情在 `s_emotions[]` 中的顺序，就是关闭随机模式后长按或上滑的切换顺序；打开 `FACE MOODS / RANDOM` 后，新表情也会自动进入随机池。

### 5. 设置表情颜色

普通表情的颜色由 `s_emotions[]` 中的 RGB 值决定，例如：

```c
/* DeepSeek 蓝 */
23, 105, 255, false

/* 红色 */
255, 55, 65, false

/* 绿色 */
56, 226, 138, false
```

`angry` 是特殊示例：资源原始时间线为圆眼、变形成愤怒眼、再恢复圆眼，代码中的 `angry_red_mix()` 按帧把它处理成“蓝色圆眼 → 渐变红色愤怒眼 → 恢复蓝色”。如果自制动画帧数不同，需要根据自己的关键帧调整 `angry_red_mix()` 中的 `8`、`58`、`66`；普通单色表情保持最后一个字段为 `false`，不需要修改时间线代码。

### 6. 编译、烧录和验证

```bash
idf.py build
idf.py -p /dev/cu.usbmodemXXXX flash monitor
```

启动后在设置页确认 `FACE MOODS` 为 `HOLD`，回到主页长按或上滑，按顺序检查所有表情。确认以下项目：

- 动画区域没有黑色矩形；
- 眼睛没有被裁切或偏离中心；
- 动画循环首尾衔接自然；
- 连续切换表情不会卡死；
- `RANDOM` 模式能随机切到新表情；
- 固件没有超过 `partitions.csv` 中的应用分区。

如果编译提示找不到 `_binary_..._start`，通常是文件名、`EMBED_FILES` 路径和 C 代码符号没有完全对应。若动画全黑，检查 GIF 是否为黑底白眼；若出现矩形，检查背景是否真的是 `#000000`；若动画速度不合适，修改 `brookesia_face_player.c` 中的 `EYE_FPS` 后重新编译。

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

表情随机轮播和页面自动轮播是两项独立的运行时设置，默认都关闭。打开 `FACE MOODS / RANDOM` 后只随机更换表情；打开 `AUTO PAGES` 后循环浏览主页、余额、Friday、日历、天气和设置页，设置页之后自动回到主页。关闭页面轮播后恢复手动左右滑动。

烧录会覆盖板子当前程序。启动后先看到 Agent 表情页；Wi-Fi 成功联网且 API Key 有效时，状态会变为 `BALANCE READY`。横向划动屏幕可进入余额页。

## 排查

- 一直显示 `CONFIG REQUIRED`：`main/agent_config.h` 里的 Wi-Fi SSID 仍为空，或这个文件没有创建。
- 显示 `WIFI OFFLINE`：检查 SSID 和密码，确认网络允许 2.4 GHz ESP32-S3 接入。
- 显示 `SYNC FAILED`：检查 API Key、账号余额权限和 HTTPS URL。串口日志不会打印 API Key。
- 长按或上滑后表情没有变化：确认设置页 `FACE MOODS` 是 `HOLD`；`RANDOM` 模式由后台任务自动切换，不响应手动切换。
- 屏幕不亮或触摸错误：先烧录相邻的 [`esp32s3-co5300-osptek-lvgl9`](../esp32s3-co5300-osptek-lvgl9/README.md) 原厂基准例程验证硬件。
