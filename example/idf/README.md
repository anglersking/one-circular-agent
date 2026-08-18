# ESP-IDF Examples

这里的每个子目录都是一个独立、可单独构建和烧录的 ESP-IDF 工程。它们不共享 `main/`、`components/`、`sdkconfig`、`dependencies.lock` 或 `build/`，因此加入新的开发板、显示屏或功能例程时不会影响已有例程。

| 例程目录 | 硬件与用途 | 文档 |
| --- | --- | --- |
| [`esp32s3-co5300-osptek-lvgl9/`](esp32s3-co5300-osptek-lvgl9/) | OSPTEK 原厂 ESP32-S3 + CO5300 + CST820 + LVGL 9 widget demo，作为屏幕和触摸硬件基准 | [来源、接线、编译与烧录](esp32s3-co5300-osptek-lvgl9/README.md) |
| [`esp32s3-co5300-deepseek-quota/`](esp32s3-co5300-deepseek-quota/) | ESP32-S3 + CO5300 + CST820，像素 Agent 表情、Wi-Fi 和 DeepSeek 余额 / 柱状历史 | [本地配置、接线、编译与烧录](esp32s3-co5300-deepseek-quota/README.md) |
| [`esp32s3-co5300-deepseek-featuretest/`](esp32s3-co5300-deepseek-featuretest/) | DeepSeek 余额例程的隔离实验副本，用于试验蓝色 ElectronBot 风格眼睛动画 | [实验边界、配置、编译与烧录](esp32s3-co5300-deepseek-featuretest/README.md) |
| [`esp32s3-co5300-grok-agent/`](esp32s3-co5300-grok-agent/) | ESP32-S3 + CO5300 + CST820，纯本地 LVGL blob Agent、多状态眼睛和点击交互；无 Wi-Fi、API 或余额功能 | [工程边界、接线、编译与烧录](esp32s3-co5300-grok-agent/README.md) |
| [`esp32s3-co5300-electronbot-face/`](esp32s3-co5300-electronbot-face/) | ESP32-S3 + CO5300 + CST820，ElectronBot 风格黑底白眼 Lottie/GIF 全屏动画；无 Wi-Fi、API 或余额功能 | [来源、接线、编译与烧录](esp32s3-co5300-electronbot-face/README.md) |
| [`esp32s3-co5300-agent-watch/`](esp32s3-co5300-agent-watch/) | ESP32-S3 + OSPTEK 1.73 英寸 CO5300 AMOLED + CST820，ONE 圆形智能体显示与触摸底座 | [接线、编译与烧录](esp32s3-co5300-agent-watch/README.md) |

## 使用一个例程

进入目标例程目录后再执行 ESP-IDF 命令，不能在 `example/idf/` 索引目录中直接构建：

```bash
cd example/idf/esp32s3-co5300-agent-watch
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/cu.usbmodemXXXX flash monitor
```

每个例程的 README 会额外说明实际 GPIO、支持的 ESP-IDF 版本、分区要求和串口日志。`build/`、`managed_components/`、`sdkconfig` 是本机生成文件，已被 Git 忽略。

## 新增例程的约定

新建例程时创建 `example/idf/<board-or-feature>/`，例如 `example/idf/esp32s3-rgb-panel/`。该目录必须自行包含：

- 顶层 `CMakeLists.txt` 与 `main/`
- 自己的 `components/`（需要的本地驱动、UI 或协议组件）
- `idf_component.yml`、`dependencies.lock` 和 `sdkconfig.defaults*`
- 独立的 `README.md`，至少写明硬件、接线、`set-target`、编译、烧录、串口监视和已验证版本

如多个例程未来确实需要复用稳定代码，可在仓库另建明确版本化的共享组件目录；在此之前，不要让一个例程用相对路径引用另一个例程的 `main/` 或 `components/`。
