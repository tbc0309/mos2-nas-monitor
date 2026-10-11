# MOS2-NAS MOS2模拟器

## Windows 免安装版

下载 Release 中 `MOS2-NAS-Simulator-1.1.1-Windows.zip`，完整解压后双击 `MOS2-NAS-Simulator.exe`。程序自动打开本机浏览器；关闭控制台窗口停止模拟器。不要单独移动 exe，`_internal` 目录也需要保留。

- 屏幕按240×240实际像素显示，可测试页面、触摸多击、闹钟、相册及管理配置。
- 管理页密码默认 `12345678`，NAS 地址和 community 在管理页填写，可访问电脑所在网络中的真实 SNMP NAS。
- 照片保存在解压目录 `data`；模拟设置只在当前运行期间有效。
- 浏览器 `/setup` 页面展示配网操作，网络列表是示例，不会让电脑连接对应 Wi-Fi。
- 模拟器与发布固件共用 C++ 绘图代码，发布包同时包含对应固件与校验信息。
- **模拟器不执行 ESP8266 bin。替换 bin 不会改变模拟画面；测试新版请下载对应模拟器包，或从对应源码重新构建。**
- SPI刷新速度、无线信号、蜂鸣器声音和设备启动仍需实体MOS2验证。

端口自动选择，避免与其他模拟器冲突。也可命令行指定：

```powershell
.\MOS2-NAS-Simulator.exe --port 8765 --no-browser
```

## 源码运行与打包

源码运行方法见项目 README 的模拟器章节。Windows打包需要 Python、MinGW g++，执行：

```powershell
python -m pip install -r simulator/requirements.txt
python -m pip install -r simulator/requirements-build.txt
python tools/package_simulator.py
```

先完成固件编译。打包工具同时构建静态链接的原生绘图程序，产物在 `dist`，不需要用户安装 Python 或编译器。
