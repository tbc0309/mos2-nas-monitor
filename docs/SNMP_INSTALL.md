# 安装与配置 SNMP

[返回首页](../README.md) · [实测支持](COMPATIBILITY.md) · [v3 参数](SNMPV3.md)

群晖、威联通、铁威马等已有厂商 SNMP 开关的设备优先使用系统管理界面。本页用于没有开启 SNMP 的 Ubuntu、Debian 系 fnOS、OpenWrt 和 iStoreOS。

脚本配置 **v2c 只读访问、UDP 161，仅允许指定的 MOS2 IPv4 地址**，生成随机社群并在完成后显示。建议在路由器中为 MOS2 固定 DHCP 地址。已有 SNMP 配置会备份一次，然后替换为 MOS2 专用配置；如果其他监控系统也在使用 SNMP，请手工合并配置。

## Ubuntu / fnOS

先把 `192.168.1.50` 换成 MOS2 的实际 IP；在被监控机器的 SSH 终端执行：

```sh
curl -fL https://raw.githubusercontent.com/tbc0309/mos2-nas-monitor/v1.2.0/scripts/snmp/install-debian.sh -o /tmp/mos2-snmp-install.sh
sudo env MONITOR_IP=192.168.1.50 sh /tmp/mos2-snmp-install.sh
```

需要 root/sudo、可用的 apt 软件源和网络。脚本安装 `snmpd` 与 `snmp`，配置开机启动；UFW 已开启时增加仅允许 MOS2 的 UDP 161 规则。其他防火墙请放行同一来源。

## OpenWrt / iStoreOS

以 root 登录被监控机器，修改 MOS2 IP 后执行：

```sh
uclient-fetch -O /tmp/mos2-snmp-install.sh https://raw.githubusercontent.com/tbc0309/mos2-nas-monitor/v1.2.0/scripts/snmp/install-openwrt.sh
MONITOR_IP=192.168.1.50 sh /tmp/mos2-snmp-install.sh
```

如果系统没有 `uclient-fetch`，第一行可以改用上节的 `curl -fL … -o …`。脚本根据系统选择 `opkg` 或 `apk`，使用 UCI 配置服务，只添加 LAN 入站规则，不开放 WAN。

## 在 MOS2 中填写

1. 打开管理页的 **设置 → SNMP 配置**，添加设备。
2. 地址填写被监控机器 IP，版本选择 **v2c**，端口 **161**。
3. 社群填写安装脚本最后显示的 `Community`，网络接口先选自动，保存。
4. 进入总览等待首次采样；网络速率需要两个计数器样本，硬盘与卷稍后独立发现。

社群不是设备管理密码。v2c 不加密；需要认证与隐私保护时使用设备支持的 [v3](SNMPV3.md)。不要把真实社群、用户名或密码提交到 Issues。

## 可选：已有内核传感器的 CPU 温度

**没有温度数据时才考虑此步骤**，不需要安装 sensors、SMART 代理或自定义守护进程。下载的脚本只读取内核已提供的 `coretemp/k10temp/CPU thermal` 接口；不存在真实 CPU 传感器时返回未知。不会把硬盘、ACPI 主板温度或负值当作 CPU 温度。

OpenWrt / iStoreOS：

```sh
uclient-fetch -O /usr/bin/mos2-cpu-temp https://raw.githubusercontent.com/tbc0309/mos2-nas-monitor/v1.2.0/scripts/snmp/read-cpu-temperature.sh
chmod 755 /usr/bin/mos2-cpu-temp
/usr/bin/mos2-cpu-temp
uci set snmpd.cpu_temp=extend
uci set snmpd.cpu_temp.name='mos2CpuTemp'
uci set snmpd.cpu_temp.prog='/usr/bin/mos2-cpu-temp'
uci commit snmpd
/etc/init.d/snmpd restart
```

Ubuntu / fnOS：

```sh
curl -fL https://raw.githubusercontent.com/tbc0309/mos2-nas-monitor/v1.2.0/scripts/snmp/read-cpu-temperature.sh -o /tmp/mos2-cpu-temp
sudo install -m 755 /tmp/mos2-cpu-temp /usr/local/bin/mos2-cpu-temp
sudo /usr/local/bin/mos2-cpu-temp
sudo sed -i '/^extend .*mos2CpuTemp /d' /etc/snmp/snmpd.conf
echo 'extend mos2CpuTemp /usr/local/bin/mos2-cpu-temp' | sudo tee -a /etc/snmp/snmpd.conf
sudo systemctl restart snmpd
```

`35000` 表示 35°C。空输出/非零退出状态表示没有可用 CPU 传感器，不应填固定数字。MOS2 会缓存“不支持”的能力判断，修改代理后切换目标再切回来或重启 MOS2 重新发现。

## 排查与恢复

- 超时：检查代理服务、防火墙、MOS2 实际 IP 是否与允许来源一致。
- 基本名称有值、其他字段未知：检查 SNMP view 是否只开放了 system 子树；本项目还需要 HOST-RESOURCES、IF-MIB 与 UCD-SNMP。
- `No Such Object`：该代理/编译版本未实现相应 OID，不能靠缩短刷新间隔解决。
- OpenWrt 存储卷有值、硬盘为空：可能未编译通用硬盘/diskIO 表；不能把存储卷伪装成物理硬盘。
- 健康未知：标准 SNMP 容量/磁盘表不等于 SMART。厂商专用表未提供时只能显示基础信息。

原配置备份：Debian 在 `/etc/snmp/snmpd.conf.mos2-backup`，OpenWrt 在 `/etc/config/snmpd.mos2-backup`。恢复后重启 `snmpd`。移除 OpenWrt 专用防火墙规则可执行 `uci delete firewall.mos2_snmp; uci commit firewall; /etc/init.d/firewall reload`。

配置依据：[Net-SNMP 官方配置手册](https://www.net-snmp.org/docs/man/snmpd.conf.html)、[OpenWrt 官方服务脚本](https://github.com/openwrt/packages/blob/master/net/net-snmp/files/snmpd.init)。安装器测试使用隔离目录与模拟服务命令，不会为了测试重装现有设备上的代理。
