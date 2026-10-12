# MOS2-NAS MOS2模拟器

## Windows 免安装版

下载 Release 中 `MOS2-NAS-Simulator-1.2.0-Windows.zip`，完整解压后双击 `MOS2-NAS-Simulator.exe`。程序自动打开本机浏览器；关闭控制台窗口停止模拟器。不要单独移动 exe，`_internal` 目录也需要保留。

- 屏幕按240×240实际像素显示，可测试页面、触摸多击、闹钟、相册及管理配置。
- 管理页密码默认 `12345678`，NAS 地址和 community 在管理页填写，可访问电脑所在网络中的真实 SNMP NAS。
- 照片保存在解压目录 `data`；配置加密保存，重启后仍保留。
- 浏览器 `/setup` 页面展示配网操作，网络列表是示例，不会让电脑连接对应 Wi-Fi。
- 模拟器与发布固件共用 C++ 绘图代码，发布包同时包含对应固件与校验信息。
- **模拟器不执行 ESP8266 bin。替换 bin 不会改变模拟画面；测试新版请下载对应模拟器包，或从对应源码重新构建。**
- SPI刷新速度、无线信号、蜂鸣器声音和设备启动仍需实体MOS2验证。

端口自动选择，避免与其他模拟器冲突。也可命令行指定：

```powershell
.\MOS2-NAS-Simulator.exe --port 8765 --no-browser
```

## 源码运行与打包

在项目根目录安装 Python 依赖和 C++17 编译器：

```shell
python -m pip install -r simulator/requirements.txt
g++ -std=c++17 -O2 -static -Iinclude -Isimulator/native simulator/native/renderer.cpp -o simulator/renderer.exe
python simulator/server.py --port 8878 --no-browser
```

屏幕地址 `http://127.0.0.1:8878/`，管理地址 `/manage`。修改绘图代码后重编译 renderer。发布前在本地验证页面、配置保存及协议错误场景。

Windows 打包执行：

```powershell
python -m pip install -r simulator/requirements.txt
python -m pip install -r simulator/requirements-build.txt
python tools/package_simulator.py
```

先完成固件编译。打包工具同时构建静态链接的原生绘图程序，产物在 `dist`，不需要用户安装 Python 或编译器。

字体与固件共用 `data/fonts/` 中的目录标识和 16 个分类/字号资源文件，必须完整复制。开发测试可用 `MOS2_FONT_DIRECTORY` 指定资源目录；不能混用其他构建的字库。分类与故障隔离见 [字库说明](../docs/FONTS.md)。

## 源码职责

`snmp_profiles.py` 负责系统分类、通用指标和能力备用路径，与固件的 `include/snmp/` 对齐；`server.py` 负责请求预算、采集调度及 HTTP 路由。`configuration.py` 负责配置规范化和原子存储，`credentials.py` 负责凭据，`security.py` 负责会话访问策略。采集器与绘图互相独立，渲染不能发起 SNMP 请求。

后台源码位于 `web/management/`，运行 `python tools/embed_web.py` 后生成单个 `web/management.js`，不要直接改生成文件。新增系统需同步响应缺失、协议错误、超时、表边界和目标切换测试；本地测试代码不包含在公开仓库。
