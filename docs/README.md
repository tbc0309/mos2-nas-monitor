# 文档目录

日常安装、配网和操作见 [项目 README](../README.md)。本目录保留配置与开发所需的补充说明。

| 文档 | 内容 |
| --- | --- |
| [3D 打印外壳](3D_PRINT_CASE.md) · [English](3D_PRINT_CASE.en.md) | 电视造型外壳 STL 下载、部件清单、渲染图与打印说明 |
| [实测支持](COMPATIBILITY.md) | 六台实际环境的指标、名称、硬盘/卷及温度边界 |
| [SNMP 安装](SNMP_INSTALL.md) · [English](SNMP_INSTALL.en.md) | Ubuntu / fnOS / OpenWrt / iStoreOS 复制执行的只读代理配置 |
| [SNMP 配置](SNMPV3.md) | v2c / v3 参数、认证与加密、状态标识 |
| [数据采集](COLLECTION.md) | 采集周期、缓存、一致性、重试与诊断 |
| [文件系统与升级](FILESYSTEM.md) | Flash 布局、统一固件、升级保留数据、字库资源 |
| [安全设计](SECURITY.md) | 登录保护、测试范围、SDK 核对及安全边界 |
| [代码结构](STRUCTURE.md) | 模块职责、语言包与资源生成 |
| [系统适配](TELEMETRY.md) | 通用能力、系统识别、专用适配与扩展步骤 |
| [配置参数](PARAMETERS.md) | 完整字段映射、命名、默认值、约束与应用方式 |
| [字库资源](FONTS.md) | 分类文件、按需读取、固定缓存、故障隔离与压力测试 |
| [绘图与刷新](RENDERING.md) | 分块、脏区域、缓存及页面切换验证 |
| [固件开发](DEVELOPMENT.md) | 构建、修改入口、回归、串口验证与硬件引脚 |

贡献流程见 [CONTRIBUTING](../CONTRIBUTING.md)。模拟器运行、开发与打包仅放在 [simulator/README.md](../simulator/README.md)。

`images/` 保存项目 README 使用的界面预览。
