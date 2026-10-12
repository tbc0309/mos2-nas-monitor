# SNMP 实测支持

[返回首页](../README.md) · [安装代理](SNMP_INSTALL.md) · [适配原理](TELEMETRY.md)

以下是 2026-10-12 实际 SSH 与 SNMP 核对的结果，不表示同品牌所有型号都会暴露相同字段。健康、温度和硬盘型号取决于设备自身的 SNMP 服务；系统后台能显示，不代表 SNMP 一定提供。

| 实测系统 | CPU / 内存 | 网络速率 | 系统运行时间 | 温度 | 硬盘信息 | SMART 健康 | 存储卷容量 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 群晖 DS920+ | 支持 | 支持 | 支持 | 群晖系统温度 | 4 块，型号与温度 | 支持 | 顶层 `/volumeN` |
| fnOS，x86_64 | 支持 | 支持 | 支持 | CPU Package | SATA＋USB 整盘名称 | 未暴露 | 系统卷、数据卷、已挂载 USB |
| Ubuntu，x86_64 | 支持 | 支持 | 支持 | CPU Package | 2 块 SATA 名称 | 未暴露 | 根目录、数据挂载 |
| iStoreOS，x86_64 | 支持 | 支持 | 支持 | 已配置的 CPU 温度扩展 | SATA＋USB，基础容量 | 未暴露 | 系统卷、`/mnt/data` |
| OpenWrt，aarch64 | 支持 | 支持 | 支持 | 当前代理提供的传感器温度 | 当前 SNMP 未暴露 eMMC | 未暴露 | 当前代理提供的磁盘挂载 |
| 铁威马 TOS，x86_64 测试环境 | 支持 | 支持 | 支持 | 当前代理暴露的 ACPI 温度 | 1 块 SATA，基础容量 | 未暴露 | 当前代理提供的磁盘挂载 |

本轮协议：群晖使用 v3 MD5＋DES，其余使用 v2c。MD5 / SHA 与 DES / AES 四种组合有协议回归测试及此前群晖实测；不据此假定其他代理默认启用了 v3。

## 名称与系统类型

1. 管理页填写设备名称时直接使用自定义名称；留空时自动获取。
2. 从 `sysDescr.0` 的标准 uname 描述提取主机名。只接受 ASCII 字母、数字、点、横线与下划线，最多 64 字符。
3. 描述无法解析、主机名包含中文/CJK 或控制字符时，再查询 `sysName.0`。它同样需要通过字符检查；都不安全时显示 `SNMP`。
4. 群晖型号来自专用 Model OID，例如 `MRTBC (DS920+)`。其他系统类型由描述、企业 OID 或专用响应确定，不根据主机名猜测。

例如本轮自动名称为 `trim-a7c9 (Linux)`、`666 (Linux)`、`TNAS123 (Linux)`、`JDCloud (Linux)`、`MRTBC (DS920+)`、`nuc (Ubuntu)`。前四台只返回通用 Linux 描述，固件不会把手工标签当作系统识别证据。

## 硬盘与存储卷

- 群晖使用专用硬盘表，标准表内的 RAID / LVM 不重复计数。
- 通用系统先读 HOST-RESOURCES 硬盘表，未找到整盘时读取 UCD diskIOTable。
- 保留 `sd/hd/vd/xvd`、NVMe、eMMC、SATA 整盘；排除分区、RAID `md`、LVM `dm`、loop、zram、nbd 和 eMMC 启动区域。
- 铁威马的 `md0` 位于 `sda4` 上，是逻辑 RAID；实际只有 `sda` 一块物理盘。
- 群晖只显示 `/volumeN` 和 `/volumeUSBn/usbshare…` 顶层挂载，不查询其中普通目录或绑定挂载的容量。SNMP WALK 的原始名称表仍可能包含这些条目。
- 群晖存储卷容量不是存储池物理总容量。SHR/RAID 池可能包含多个卷，不能把卷名当作存储池；当前页面展示文件系统可用容量。
- USB 盘必须已挂载且由 SNMP 暴露容量，才会出现在存储卷页面；不通过目录名称猜测 USB 类型。群晖/TOS 本轮未接外置盘，相关路径筛选通过组件测试。

## 温度与健康的边界

iStoreOS 本轮没有 LM-SENSORS 温度表，但内核已有 `coretemp`。现有 `mos2CpuTemp` 扩展只读 `temp1_input`，实际 CPU 核心从 `temp2_input` 开始，导致空值。现已修正：优先 Package / Tdie / Tctl，缺失时取 CPU 核心最高值；不读取硬盘传感器冒充 CPU 温度。扩展使用已存在的内核接口，不安装驱动或额外采集进程。

Net-SNMP 通用硬盘表不提供完整 SMART。没有专用健康字段时显示基础盘信息和未知健康，不能把“能读取容量”当作“硬盘健康正常”。TOS 测试环境只暴露 ACPI 温度，不将其标注为 CPU 核心温度。

威联通、其他铁威马型号、不同内核及 ARM 设备需按实际 MIB 再验证；新增系统的入口见 [适配开发](TELEMETRY.md)。
