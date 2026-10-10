# MOS2-NAS

**版本 1.0.0** · MOS2 小电视的桌面与群晖 NAS 监控固件。

通过 SNMP v2c 获取 CPU、内存、网络速率、系统温度、硬盘健康与存储卷信息，无需安装 Netdata。支持中文 / English、扫码配网、网页管理、天气、相册与蜂鸣器闹钟。

目标硬件是 **ESP8266 / ESP-12F、4MB Flash、ST7789 240×240**，不是通用 ESP32 固件。

## 界面预览

以下屏幕预览使用示例数据，展示实际固件绘制代码的输出。

![NAS 六个页面](docs/images/nas-pages.png)

原版大时钟数字保留，其他屏幕文字使用腾讯 W7 点阵；默认时白、分橙，后台可独立调色。天气页保留原版动画。

<table width="100%">
<tr><th width="50%">大字时钟（开启闹钟）</th><th width="50%">天气与时间</th></tr>
<tr><td align="center"><img src="docs/images/desktop-clock.png" width="360" height="360" alt="大字时钟模拟器导出"></td><td align="center"><img src="docs/images/weather-clock.png" width="360" height="360" alt="天气模拟器导出"></td></tr>
</table>

### 后台管理页面

在浏览器中设置桌面显示、天气、闹钟、NAS 连接和相册，并查看设备状态。

![后台管理页面](docs/images/web-management.png)

## 功能与操作

桌面组：**时钟 → 天气 → 相册**。

NAS 组：**总览 → 网络 → CPU / 内存 → 硬盘健康 → 存储卷 → 设备状态**。

| 连续短按 | 操作 |
| --- | --- |
| 单击 | 下一页；管理或配网页面返回 |
| 双击 | 相册下一张；硬盘或存储卷下一组 |
| 三击 | 切换桌面 / NAS 两组功能 |
| 五击 | 管理二维码 |
| 六击 | 重新配网 / 热点二维码 |
| 八击 | 进入重置确认，再独立三击执行 |

四击、七击与长按不触发功能。相邻点击间隔保持短于约 350ms；最后一次松开后，设备等待整组点击结束再执行。重置确认十秒后自动取消，其他点击取消确认。

闹钟响铃或贪睡时，**单击贪睡、三击停止当天闹钟**优先于普通翻页操作。

## 初次使用与配网

1. 连接 USB 电源。首次启动没有网络配置时进入配网热点；已有配置时自动联网，连接失败约 30 秒后进入热点。
2. 扫描屏幕 Wi-Fi 二维码加入 `MOS2-NAS-xxxxxx`，默认热点密码 `12345678`。
3. 打开 `http://192.168.4.1/`，扫描并选择附近的 2.4GHz Wi-Fi，填写密码并保存（隐藏网络可手动输入）；联网后再配置 NAS。
4. 五击打开管理二维码，管理页只需密码，默认 `12345678`。会话闲置30分钟或重启后重新登录。

设备支持 SNMP v2c / HTTP，适合可信局域网使用，不建议直接暴露到公网。

## 群晖 NAS 设置

在 DSM 的 **控制面板 → 终端机和 SNMP → SNMP** 中启用 SNMPv1 / SNMPv2c，设置 community。在设备后台填写 NAS 的 IPv4 地址、community 与 UDP 端口，默认端口为 `161`。留空密码类字段表示保留原值。

网络接口默认自动选择已连接的网口。SNMP 连接后，管理页提供 NAS 网口名称和连接状态，可按需选择，例如 `eth0`。

- CPU 使用率根据 UCD-SNMP 原始计数器差值计算，空闲与 I/O 等待不计入忙碌时间。
- 内存使用率扣除空闲、buffers 和 cache，尽量贴近 DSM 资源监控口径；采样时刻不同仍可能有差异。
- 网络速率使用 IF-MIB Counter64 差值，红色为 NAS 发送、绿色为 NAS 接收，单位为字节每秒。
- 温度来源为群晖系统温度 OID。圆环低于 60°C 为绿色，60–74°C 为橙色，75°C 起为红色；这是固件提示阈值，**不是 DSM 的自动报警设置**。
- 最多显示 12 块硬盘、8 个 `/volumeN` 存储卷；硬盘每组 4 块、存储卷每组 3 个，双击翻组。
- 缓存已有数据并异步刷新，元数据与动态指标采用不同刷新频率。未知字段显示未知，不伪造健康状态。

当前不支持 SNMPv3，也不依赖 Netdata。

## 网页配置

- **桌面与显示**：屏幕语言、启动页面、时分颜色、秒数开关、亮度、方向、触摸提示音。
- **天气与时间**：默认天津 `101030100`；天气刷新 5–60 分钟，NTP 默认 `ntp.aliyun.com`，时间使用 UTC+8。
- **闹钟**：选择周一至周日、时间、四首 8-bit 铃声与 1–30 分钟贪睡时长。NTP 尚未同步时不触发定时闹钟。
- **相册播放**：自动轮播或固定显示，固定模式双击切换照片。
- **NAS**：底色、地址、community、端口、网络接口与刷新间隔。
- **相册管理**：拖动裁剪、缩放、旋转、自动选择空位或明确替换已有照片。

相册最多 **7 张**，保存为 **240×240 JPEG，每张不超过 48KB**。升级程序保留照片；恢复出厂设置会删除配置与全部照片。

## 编译与刷入

安装 Python 3 与 PlatformIO Core 后，在仓库根目录执行：

```shell
python -m pip install platformio
pio run
```

产物为 `.pio/build/mos2/firmware.bin`。项目已包含生成好的字体与网页头文件，普通编译无需安装腾讯字体。

使用 USB 转串口刷入程序，替换示例中的 `COM7` 为实际端口：

```shell
python -m pip install esptool
python -m esptool --port COM7 --baud 460800 write-flash 0x0 .pio/build/mos2/firmware.bin
```

**升级时仅写程序，不全片擦除、不覆盖 LittleFS 分区**，以保留配置和相册。首次尝试前建议备份原设备固件；仓库不包含任何设备完整备份或私人配置。

## 本地模拟器

模拟器与固件共用 C++ 绘图代码。安装 Python 依赖和 C++17 编译器：

```shell
python -m pip install -r simulator/requirements.txt
g++ -std=c++17 -O2 -Iinclude -Isimulator/native simulator/native/renderer.cpp -o simulator/renderer.exe
python simulator/server.py
```

浏览器打开 `http://127.0.0.1:8765/` 查看屏幕，`http://127.0.0.1:8765/manage` 查看管理页。屏幕按 240×240 原始像素显示。默认 NAS 地址是占位配置，需要在后台填写自己的 NAS 参数。

模拟器不播放实体蜂鸣器声音，配置主要保存在内存中；模拟器的触摸按钮与实机电极灵敏度也不同。本地服务只监听回环地址。

## 硬件引脚

| 功能 | GPIO |
| --- | --- |
| TFT SCK | 14 |
| TFT MOSI | 13 |
| TFT DC | 0 |
| TFT RESET | 2 |
| TFT CS | 接地，配置为 -1 |
| 背光 | 5，低有效 |
| TTP223 触摸 | 4，低有效 |
| 蜂鸣器 | 12 |

USB 转串口为 CH340，供电与电平应符合 MOS2 原板设计。

## 修改界面与字库

网页源文件位于 `web/`，前后端文字分别维护于 `locales/` 和 `include/locales/`。修改后重新生成：

```shell
python tools/embed_locales.py
python tools/embed_web.py
```

屏幕字库通过 FreeType 单色自动网格对齐生成，处理字形边界、基线和独立字号。重建腾讯 W7 字库需要自行准备有权使用的字体文件，Windows PowerShell 示例：

```powershell
python -m pip install -r tools/font-requirements.txt
$env:FONT_PATH = 'C:/Fonts/TencentSans-W7.ttf'
python tools/build_fonts.py
```

大时钟保留原版图片数字，不经过此字体生成脚本。管理、登录和配网页面使用浏览器系统字体，与小电视屏幕字库独立。

## 验证与素材来源

已在 MOS2 ESP8266 实机验证联网、SNMP 采样、4 块硬盘 / 3 个存储卷、应用升级保留配置与照片；触摸解析、重置确认、字体边界另有主机测试。屏幕观感、触摸电极与蜂鸣器音色仍需实机确认。

硬件、原版大时钟、天气图标和动画参考用户提供的 JSRUN MOS2 / SmallDesktopDisplay 项目，NAS 界面参考 routermonitor。第三方字体与素材说明见 [NOTICE](NOTICE.md)。

## 代码结构与配置

`src/main.cpp` 负责初始化和主循环。`src/app/` 按职责分为状态、JSON 配置、显示刷新、网页 API、参数采样、触摸控制和相册等模块；`include/` 保存可独立测试的协议、字体、闹钟和绘图组件。生成字库和网页头文件由 `tools/` 维护，不手工修改。

新版使用 `/config.json`（schema 1），不读取旧项目的逐行配置文件。首次从旧固件升级需要重新配网和填写 NAS 参数；此后同一配置结构的应用更新保留设置和照片。配置写入失败会返回错误，临时文件完成后才替换正式配置。恢复出厂设置清除全部用户数据。

闹钟开启时，大时钟页在闹钟时间左侧显示橙色闹钟图标，天气时间页在秒数上方显示图标；关闭秒数显示时仍保留图标。

安装 PlatformIO 依赖并先运行 `pio run` 后，主机验证可运行 `python tools/check.py`；需要 C++17 编译器和 `simulator/requirements.txt` 中的 Python 依赖。固件编译使用 `pio run`。

桌面与天气图片由 `python tools/export_screens.py` 直接导出模拟器绘图指令，保持原始 240×240 像素，左右并排显示。

管理登录采用密码与 Cookie 会话，取消用户名和浏览器认证弹窗。默认密码仅用于新初始化设备；升级会保留已有自定义密码。模拟器运行在本机，管理页演示密码登录，屏幕与测试 API 继续提供本地开发访问。

Windows 免安装模拟器随固件发布，解压运行即可查看小电视和管理页面。说明与打包方法见 [模拟器说明](simulator/README.md)。模拟器运行共享绘图代码，不直接执行 bin；新版需使用对应的模拟器包。

配网与登录页通过右上角切换中英文，每次显示一种语言。网络保存后会显示成功反馈、网络名称及后续连接步骤；失败时保留表单方便重试。
