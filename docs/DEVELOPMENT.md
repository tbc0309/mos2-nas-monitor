# 固件开发

[返回首页](../README.md) · [模块结构](STRUCTURE.md) · [贡献说明](../CONTRIBUTING.md)

## 构建

需要 Python 3、PlatformIO。仓库包含生成好的字库与网页资源，普通构建不需要腾讯字体文件。

```sh
python -m pip install platformio==6.2.0
pio run -e mos2
pio run -e mos2 -t buildfs
python tools/build_install_image.py --output dist/MOS2-NAS-1.2.0.bin
```

最后一个文件同时包含程序和系统字库，用于 USB 全新安装；资源一致时管理页也能读取这个容器进行更新。Flash 布局及资源校验见 [FILESYSTEM](FILESYSTEM.md)。先保留原设备配置，再按测试需要决定是否清除用户数据。

## 修改入口

| 修改内容 | 源文件与说明 |
| --- | --- |
| 启动、主循环 | `src/main.cpp` |
| 页面绘制、动态刷新 | `include/dashboard.h`、`src/app/display.h`；[绘图](RENDERING.md) |
| SNMP 通用能力与系统适配 | `src/app/telemetry/`、`include/snmp/`；[适配](TELEMETRY.md) |
| 硬盘、卷、接口发现 | `include/storage_collector.h`、`include/network_interfaces.h` |
| 网页设置与数据响应 | `src/app/management/`、`web/management/` |
| 配置、凭据与会话 | `src/app/config.h`、`include/credential_store.h`；[安全](SECURITY.md) |
| 字段规范与生命周期 | [参数约定](PARAMETERS.md)、[采集](COLLECTION.md) |
| 字库与天气城市 | `tools/font_charset.py`、`tools/weather/`；[字库](FONTS.md) |

在采集器定义周期、退避、范围验证和缓存期限，提交完整样本后绘图；不要从绘制函数发起网络请求。系统专用 OID 必须有真实识别证据，不能根据主机名猜品牌。缓存属于 RAM，用户配置/照片才是持久数据。

## 网页与语言

编辑 `web/` 和 `locales/` 中的源文件，不直接修改生成的 `include/webui.h`。英文命名用于代码；用户文案进入语言包，关键生命周期用必要的注释解释。

```sh
python tools/embed_locales.py
python tools/embed_web.py
```

字库重建需要有权使用的腾讯 W7 文件，按 [FONTS](FONTS.md) 生成；修改程序与字库目录结构后同时构建程序和文件系统。

## 验证

提交前重新生成资源并构建程序、文件系统和统一固件。GitHub Actions 核对生成文件是否与源文件一致，并验证这些构建步骤。组件、协议和模拟器回归在本地执行，测试目录与运行器不包含在公开仓库中。

模拟器运行、开发和 Windows 打包说明仅放在 [simulator/README.md](../simulator/README.md)。

实体机验证还需检查串口是否出现 WDT/异常重启、空闲堆与最大连续块、后台连续保存、触摸切页、设备切换以及实际 SNMP 错误退避。`/api/diagnostics` 不包含凭据。不要把私有配置、串口里可能出现的真实地址/名称或设备凭据放入公开提交。

## 硬件

| 功能 | GPIO |
| --- | --- |
| TFT SCK / MOSI | 14 / 13 |
| TFT DC / RESET | 0 / 2 |
| TFT CS | 接地，配置 -1 |
| 背光 | 5，低有效 |
| TTP223 触摸 | 4，低有效 |
| 蜂鸣器 | 12 |

硬件为 ESP8266 / ESP-12F、4MB Flash、ST7789 240×240，USB 转串口 CH340。供电与电平按 MOS2 原板设计。
