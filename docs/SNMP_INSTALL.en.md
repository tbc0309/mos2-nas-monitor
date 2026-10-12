# Install a read-only SNMP agent

[Home](../README.en.md) · [中文](SNMP_INSTALL.md)

Use the vendor SNMP switch on Synology / QNAP / TerraMaster first. These scripts are for Ubuntu, Debian-based fnOS, OpenWrt and iStoreOS without a configured agent. They enable read-only v2c on UDP 161 for **one MOS2 IPv4 address**, generate a random community and print it when finished. Reserve a DHCP address for MOS2.

An existing SNMP configuration is backed up once, then replaced. If other monitoring systems use it, merge settings manually instead.

## Ubuntu / fnOS

Replace `192.168.1.50` with MOS2's IP and run on the monitored machine:

```sh
curl -fL https://raw.githubusercontent.com/tbc0309/mos2-nas-monitor/v1.2.0/scripts/snmp/install-debian.sh -o /tmp/mos2-snmp-install.sh
sudo env MONITOR_IP=192.168.1.50 sh /tmp/mos2-snmp-install.sh
```

Requires root/sudo and working apt repositories. Installs `snmpd` and `snmp`; enables the service. If UFW is active, permits only MOS2's UDP 161 traffic. Configure any other firewall separately.

## OpenWrt / iStoreOS

Log in as root, replace MOS2's address, then run:

```sh
uclient-fetch -O /tmp/mos2-snmp-install.sh https://raw.githubusercontent.com/tbc0309/mos2-nas-monitor/v1.2.0/scripts/snmp/install-openwrt.sh
MONITOR_IP=192.168.1.50 sh /tmp/mos2-snmp-install.sh
```

Use `curl -fL URL -o FILE` if `uclient-fetch` is unavailable. The script selects opkg/apk, configures UCI and adds a LAN-only firewall rule, never a WAN rule.

## MOS2 settings

1. Open **Settings → SNMP configuration** and add a device.
2. Enter the monitored machine's address, version **v2c**, port **161**.
3. Paste the printed `Community`, choose automatic interface and save.
4. Open overview. Network rates need two counter samples; storage discovery follows the initial metrics.

A community is not the management password. v2c has no encryption. Use supported v3 authentication/privacy when required; see [SNMP v3 (中文)](SNMPV3.md). Never post real credentials in Issues.

## Optional CPU temperature

Only add this when standard temperature OIDs are absent and the kernel already exposes a CPU sensor. The reader installs no drivers or background processes. It prefers Package / Tdie / Tctl, then the hottest available CPU core. It does not use disk or ACPI board temperature as a CPU reading.

OpenWrt / iStoreOS:

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

Ubuntu / fnOS:

```sh
curl -fL https://raw.githubusercontent.com/tbc0309/mos2-nas-monitor/v1.2.0/scripts/snmp/read-cpu-temperature.sh -o /tmp/mos2-cpu-temp
sudo install -m 755 /tmp/mos2-cpu-temp /usr/local/bin/mos2-cpu-temp
sudo /usr/local/bin/mos2-cpu-temp
sudo sed -i '/^extend .*mos2CpuTemp /d' /etc/snmp/snmpd.conf
echo 'extend mos2CpuTemp /usr/local/bin/mos2-cpu-temp' | sudo tee -a /etc/snmp/snmpd.conf
sudo systemctl restart snmpd
```

`35000` means 35°C. Empty output/nonzero status means no usable CPU sensor; do not return a fixed number. MOS2 caches unsupported capabilities: switch targets and back, or restart after changing the agent.

## Troubleshooting

- Timeout: verify the service, firewall and exact allowed source IP.
- Name only: the agent view may expose only the system subtree; IF-MIB, HOST-RESOURCES and UCD-SNMP are also needed.
- No Such Object: unsupported OID/build; faster polling cannot add missing metrics.
- Capacity without health: standard disk tables are not SMART. Missing health stays unknown.
- OpenWrt volume without physical disks: the build may omit disk/diskIO tables.

Backups: `/etc/snmp/snmpd.conf.mos2-backup` (Debian), `/etc/config/snmpd.mos2-backup` (OpenWrt). Restore the file and restart snmpd. Remove the OpenWrt rule with `uci delete firewall.mos2_snmp; uci commit firewall; /etc/init.d/firewall reload` if needed.

References: [Net-SNMP manual](https://www.net-snmp.org/docs/man/snmpd.conf.html), [official OpenWrt loader](https://github.com/openwrt/packages/blob/master/net/net-snmp/files/snmpd.init). Installer tests use isolated files and mock service commands.
