# 代码与语言包

应用只在 `src/main.cpp` 定义一次状态，按模块包含实现，避免嵌入式构建中复制缓存或网络会话。

| 目录 / 模块 | 职责 |
| --- | --- |
| `src/app/state.h`、`config.h` | 运行状态、配置验证和原子保存 |
| `src/app/configuration_types.h` | 带含义、单位的配置类型，协议字段映射见 `PARAMETERS.md` |
| `include/snmp/`、`src/app/telemetry/` | 系统证据、适配策略、能力发现、快照、调度，详见 `TELEMETRY.md` |
| `src/app/management/` | 页面与配置输出、状态响应、设置验证及应用 |
| `src/app/telemetry.h`、`monitor.h` | 独立采集任务、调度、缓存有效期 |
| `src/app/display.h` | 屏幕切换、条带刷新、动画与相册 |
| `src/app/access.h` | 配网、登录、改密、注销及异步 Wi-Fi 扫描 |
| `src/app/web.h`、`extras_web.h`、`routes.h` | 管理配置、状态 API、路由注册 |
| `src/app/firmware_update.h` | 升级验证、OTA 写入、保留或清除用户数据 |
| `include/snmp*.h` | BER、USM、MD5 / SHA、DES / AES 协议实现 |
| `include/font_resource.h`、`data/fonts/` | 字库按需读取、完整性验证和字形缓存 |
| `web/management.*` | 管理页结构、交互；浏览器默认字体 |
| `web/management/` | 设置、相册、Wi-Fi、维护源码，生成单个 `management.js` |
| `web/access.*`、`setup.js` | 登录与配网交互 |
| `web/firmware-bundle.js`、`tools/build_install_image.py` | 统一镜像校验、程序提取与镜像生成 |
| `simulator/`、`tools/snmp_v3.py` | 共享绘制代码的模拟器、主机协议采集 |

语言源文件：

- `locales/en.json`、`zh.json`：管理界面与模拟器按钮。
- `locales/access/`：登录、首次改密和配网说明。
- `locales/backend/`：设备返回的可见验证与存储提示。
- `include/locales/lcd_zh.h`：屏幕英文键到中文的映射；动态前缀在此统一处理。
- `simulator/qr-locales.json`：模拟器配网与管理二维码页面文字。

执行 `python tools/embed_locales.py` 和 `python tools/embed_web.py` 生成设备使用的资源。生成文件不直接编辑。生成器检查中英文键集合、管理标签与配网标签；完整回归由本地测试目录执行。

普通代码保持英文标识与注释；操作文档使用中文。仅对缓存、调度、公平性、密码生命周期、协议认证和文件写入这些有必要说明的约束添加注释，避免把实现逐行复述。

## 扩展入口

- 设备管理：`src/app/machines.h` 管理档案增删、选择和排序；身份变化后取消旧请求，再清缓存，避免不同机器的数据混合。
- 访问策略：`include/access_policy.h` 保存可主机测试的登录退避和严格数字解析；`simulator/security.py` 对应本地服务策略。
- 存储排序：`include/storage_order.h` 提供不溢出的自然排序；显示和 API 都按硬盘索引、卷名排序，不移动采集器内部数组，避免时间戳错配。
- 模拟器配置：`simulator/configuration.py` 负责读取限额、字段规范化和临时文件原子替换；`server.py` 处理路由及调度。可以使用 `--config` 指向仓库外私有配置。

新增采集项应定义独立周期、失败退避和有效期，完整计算后提交缓存；不得从绘制函数发起网络请求。新增页面先复用 Dashboard 的绘制入口，并增加中英文分块/整帧像素一致性验证。新增文案先更新语言源文件，再运行资源生成器并完成本地回归。

配置、照片属于持久数据，采样缓存属于 RAM。删除设备、切换目标或认证身份变化必须清除对应缓存；普通语言、颜色或页面变化不得丢弃采样基线。保存操作成功后才能修改持久状态，失败时应恢复原有配置。


凭据保护集中在 `include/credential_store.h` 与 `simulator/credentials.py`，格式和算法一致。修改算法、nonce、密钥长度或校验格式时，必须同步跨端测试；不得从绘制、采集或会话有效性检查中运行密码 KDF。新配置 schema 为 3，不读取历史明文。字体字符集由 `tools/font_charset.py` 决定，扩展清单在 `tools/font-extra.txt`。
