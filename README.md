# MOS2-NAS

**简体中文** · [English](README.en.md)

**版本 1.1.1** · MOS2的桌面与群晖 NAS 监控固件 · **MIT License**

通过 SNMP v2c / v3 获取 CPU、内存、网络速率、系统温度、硬盘健康和存储卷信息，无需安装 Netdata。同时提供天气时钟、相册、蜂鸣器闹钟、扫码配网和中英文网页管理。

适用硬件：**MOS2 · ESP8266 / ESP-12F · 4MB Flash · ST7789 240×240**。

[下载固件与模拟器](https://github.com/tbc0309/mos2-nas-monitor/releases/latest) · [快速开始](#快速开始) · [触摸操作](#触摸操作) · [开发与编译](#开发与编译)

## 界面预览

### NAS 监控

总览、网络、CPU / 内存、硬盘健康、存储卷和设备状态六个页面。以下预览由固件共用的绘图代码生成，使用示例数据。

![NAS 六个页面](docs/images/nas-pages.png)

红色表示 NAS 发送，绿色表示 NAS 接收。绿色下行位于上半区、红色上行位于下半区，各占一半高度；两条曲线分别按各自峰值缩放，用于查看变化趋势；实际速率以上方数字为准。

### 桌面与天气

保留原版大时钟数字和天气动画。默认时白、分橙，颜色可独立调整；闹钟开启时显示图标和时间。

<table width="100%">
<tr><th width="50%">大字时钟（开启闹钟）</th><th width="50%">天气与时间</th></tr>
<tr><td align="center"><img src="docs/images/desktop-clock.png" width="360" height="360" alt="大字时钟模拟器导出"></td><td align="center"><img src="docs/images/weather-clock.png" width="360" height="360" alt="天气模拟器导出"></td></tr>
</table>

### 网页管理

在浏览器中配置显示、天气、闹钟、NAS、Wi-Fi 和相册。网页使用浏览器系统字体，语言可在右上角切换。

![后台管理页面](docs/images/web-management.png)

## 快速开始

### 安装固件

从 [Releases](https://github.com/tbc0309/mos2-nas-monitor/releases/latest) 下载 `MOS2-NAS-1.1.1.bin`。同一个文件用于 USB 全新安装和网页升级，已包含程序与字库。

全新安装时，替换 `COM7` 为实际串口：

```shell
python -m pip install esptool
python -m esptool --port COM7 erase-flash
python -m esptool --port COM7 --baud 460800 write-flash --flash-mode dio --flash-size 4MB 0x0 MOS2-NAS-1.1.1.bin
```

USB 全新安装会清除配置和照片。已有设备需要保留数据时，使用管理页的固件升级功能。

### 配网与登录

1. 首次启动进入配网热点，扫描屏幕二维码加入 `MOS2-NAS-xxxxxx`，热点密码为 `12345678`。
2. 打开 `http://192.168.4.1/`，搜索并选择 2.4GHz Wi-Fi，输入密码后保存；隐藏网络可手动填写名称。
3. 等待设备联网，连续五击打开管理二维码，扫码进入管理页。
4. 默认管理密码为 `12345678`。首次登录必须修改密码，然后重新登录。

已有网络配置时自动联网；连接失败约 30 秒后进入配网热点。管理会话闲置 30 分钟或设备重启后需要重新登录。

### 连接群晖 NAS

在 DSM 的 **控制面板 → 终端机和 SNMP → SNMP** 中开启对应服务，并在设备管理页的 **NAS** 卡片填写：

| 模式 | 配置内容 |
| --- | --- |
| SNMP v2c | NAS 地址、Community、端口 |
| SNMP v3 | NAS 地址、用户名、认证协议与密码；需要加密时再填写隐私协议与密码 |

SNMP v3 支持 **MD5 / SHA** 认证和 **DES / AES** 隐私加密。端口默认 `161`，网络接口默认自动选择；连接后可从列表选择具体网口。密码类字段留空表示保留原值。

详细配置见 [SNMP v3 说明](docs/SNMPV3.md)。设备通过局域网 HTTP 提供管理页面，请在可信局域网中使用。

## 触摸操作

桌面组：**时钟 → 天气 → 相册**。

NAS 组：**总览 → 网络 → CPU / 内存 → 硬盘健康 → 存储卷 → 设备状态**。

| 连续短按 | 操作 |
| --- | --- |
| 单击 | 下一页；从管理或配网页面返回 |
| 双击 | 下一张照片；硬盘或存储卷下一组 |
| 三击 | 切换桌面 / NAS 功能组 |
| 五击 | 打开管理二维码 |
| 六击 | 重新配网，打开热点二维码 |
| 八击 | 进入重置确认，再独立三击执行 |

相邻点击间隔短于约 350ms，最后一次松开后等待整组识别。四击、七击和长按不触发功能。重置确认十秒后自动取消，其他点击也会取消。

闹钟响铃或贪睡时，**单击贪睡、三击停止当天闹钟**优先于翻页。

## 功能配置

| 管理区域 | 可设置内容 |
| --- | --- |
| 桌面与显示 | 屏幕语言、启动页面、时分颜色、秒数、亮度、方向、触摸提示音 |
| 天气与时间 | 城市代码及查询链接、天气刷新间隔、NTP 服务器 |
| 闹钟 | 时间、周一至周日、四首 8-bit 铃声、贪睡时长 |
| 相册播放 | 自动轮播、固定显示、停留时间 |
| NAS | 背景色、SNMP 参数、网络接口、采集间隔 |
| Wi-Fi / 管理密码 | 搜索网络、查看当前连接、修改 Wi-Fi 和管理密码 |
| 相册管理 | 上传、拖动裁剪、缩放、旋转、选择空位或替换照片 |
| 维护 | 固件升级、保留数据选项、恢复出厂设置 |

天气默认天津 `101030100`，刷新间隔 5–60 分钟。NTP 默认 `ntp.aliyun.com`，时区 UTC+8；时间未同步时不触发定时闹钟。闹钟默认工作日，贪睡可设 1–30 分钟。

相册最多 **7 张**，每张为 **240×240 JPEG，不超过 48KB**。固定显示模式下双击切换照片。

### NAS 数据说明

- CPU 根据原始计数器差值计算，空闲和 I/O 等待不计入忙碌时间。
- 内存扣除空闲、buffers 和 cache，尽量贴近 DSM 资源监控；采样时刻不同可能有差异。
- 网络速率根据 IF-MIB Counter64 差值计算，单位为字节每秒。
- 系统温度来自群晖 SNMP。圆环低于 60°C 为绿色，60–74°C 为橙色，75°C 起为红色；这是固件提示阈值，不代表 DSM 的报警阈值。
- 最多显示 12 块硬盘、8 个 `/volumeN` 存储卷；每组 4 块硬盘或 3 个卷，双击翻组。
- 各类数据独立采集、缓存和重试；未知字段显示未知。

## 升级与重置

管理页 **维护 → 固件升级** 选择同一个 `MOS2-NAS-1.1.1.bin`，默认勾选“保留配置和照片”。取消勾选后，验证并升级成功才清除用户数据；校验失败不清除数据。

![固件升级页面](docs/images/web-maintenance.png)

网页会校验完整文件，提取程序区升级并保留兼容的系统资源。旧版升级页无法识别统一文件，或字库资源不匹配时，需要通过 USB 安装。分区与校验机制见 [文件系统与升级](docs/FILESYSTEM.md)。

恢复出厂设置删除配置、管理密码和全部照片，保留系统字库；重启后重新配网。

## 模拟器

### Windows 免安装版

下载 [Release](https://github.com/tbc0309/mos2-nas-monitor/releases/latest) 中的 Windows 模拟器压缩包，完整解压后运行 `MOS2-NAS-Simulator.exe`，浏览器会自动打开。

模拟器可以查看屏幕与管理页，并连接电脑所在网络中的 SNMP NAS。它运行共享绘图代码，不直接执行 ESP8266 bin；替换 bin 不会改变模拟界面。蜂鸣器声音、SPI 刷新与 Wi-Fi 配网仍需实体机验证。

使用说明见 [模拟器文档](simulator/README.md)。

### 源码运行

安装 Python 依赖和 C++17 编译器：

```shell
python -m pip install -r simulator/requirements.txt
g++ -std=c++17 -O2 -static -Iinclude -Isimulator/native simulator/native/renderer.cpp -o simulator/renderer.exe
python simulator/server.py
```

打开 `http://127.0.0.1:8765/` 查看屏幕，`http://127.0.0.1:8765/manage` 查看管理页。修改绘图代码后需要重新编译 renderer。本地服务只监听回环地址。

## 开发与编译

### 构建固件

项目包含生成好的字库和网页头文件，普通编译无需安装腾讯字体：

```shell
python -m pip install platformio
pio run
pio run -t buildfs
python tools/build_install_image.py --output MOS2-NAS-1.1.1.bin
```

### 代码与语言包

`src/main.cpp` 负责启动与主循环；`src/app/` 按显示、采集、配置、网页、触摸和升级分模块；`include/` 保存协议与绘图组件。

网页源文件位于 `web/`，语言包位于 `locales/` 和 `include/locales/`。修改语言或网页后执行：

```shell
python tools/embed_locales.py
python tools/embed_web.py
```

详细说明：[代码结构](docs/STRUCTURE.md) · [数据采集](docs/COLLECTION.md) · [文件系统](docs/FILESYSTEM.md)。

### 字库与验证

屏幕使用腾讯 W7 原生 hint / 16 级灰阶字库，按需读取并缓存；大时钟保留原版图片数字。重建字库需准备有权使用的字体文件，PowerShell 示例：

```powershell
python -m pip install -r tools/font-requirements.txt
$env:FONT_PATH = 'C:/Fonts/TencentSans-W7.ttf'
python tools/build_fonts.py
```

安装模拟器依赖、C++17 编译器和 Node.js，并完成 `pio run` 后，执行 `python tools/check.py` 进行组件和模拟器测试。桌面与天气预览由 `python tools/export_screens.py` 导出绘图指令生成。

### 硬件引脚

| 功能 | GPIO |
| --- | --- |
| TFT SCK / MOSI | 14 / 13 |
| TFT DC / RESET | 0 / 2 |
| TFT CS | 接地，配置为 -1 |
| 背光 | 5，低有效 |
| TTP223 触摸 | 4，低有效 |
| 蜂鸣器 | 12 |

USB 转串口为 CH340，供电与电平按 MOS2 原板设计。

## 开源协议与来源

本项目原创代码采用 **MIT 开源协议**。第三方依赖、字体和参考素材遵循各自的许可，详见 [NOTICE](NOTICE.md)。

硬件、大时钟、天气图标和动画参考 JSRUN MOS2 / SmallDesktopDisplay，NAS 界面参考 routermonitor。

- [MOS2 官方](https://jsrun.net/hw/mos2)
- [我不是矿神](https://imnks.com/)
