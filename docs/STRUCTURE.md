# 代码与语言包

应用只在 `src/main.cpp` 定义一次状态，按模块包含实现，避免嵌入式构建中复制缓存或网络会话。

| 目录 / 模块 | 职责 |
| --- | --- |
| `src/app/state.h`、`config.h` | 运行状态、配置验证和原子保存 |
| `src/app/telemetry.h`、`monitor.h` | 独立采集任务、调度、缓存有效期 |
| `src/app/display.h` | 屏幕切换、条带刷新、动画与相册 |
| `src/app/access.h` | 配网、登录、改密、注销及异步 Wi-Fi 扫描 |
| `src/app/web.h`、`extras_web.h`、`routes.h` | 管理配置、状态 API、路由注册 |
| `src/app/firmware_update.h` | 升级验证、OTA 写入、保留或清除用户数据 |
| `include/snmp*.h` | BER、USM、MD5 / SHA、DES / AES 协议实现 |
| `include/font_resource.h`、`data/fonts/` | 字库按需读取、完整性验证和字形缓存 |
| `web/management.*` | 管理页结构、交互；浏览器默认字体 |
| `web/access.*`、`setup.js` | 登录与配网交互 |
| `web/firmware-bundle.js`、`tools/build_install_image.py` | 统一镜像校验、程序提取与镜像生成 |
| `simulator/`、`tools/snmp_v3.py` | 共享绘制代码的模拟器、主机协议采集 |

语言源文件：

- `locales/en.json`、`zh.json`：管理界面与模拟器按钮。
- `locales/access/`：登录、首次改密和配网说明。
- `locales/backend/`：设备返回的可见验证与存储提示。
- `include/locales/lcd_zh.h`：屏幕英文键到中文的映射；动态前缀在此统一处理。
- `simulator/qr-locales.json`：模拟器配网与管理二维码页面文字。

执行 `python tools/embed_locales.py` 和 `python tools/embed_web.py` 生成设备使用的资源。生成文件不直接编辑。生成器检查中英文键集合、管理标签与配网标签；完整测试入口为 `python tools/check.py`。

普通代码保持英文标识与注释；操作文档使用中文。仅对缓存、调度、公平性、密码生命周期、协议认证和文件写入这些有必要说明的约束添加注释，避免把实现逐行复述。
