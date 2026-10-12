#!/bin/sh
# OpenWrt / iStoreOS: read-only UCI configuration, no WAN firewall exception.
set -eu
[ "$(id -u)" = 0 ] || { echo 'Run as root.'; exit 1; }
: "${MONITOR_IP:?Set MONITOR_IP to the MOS2 IPv4 address}"
printf '%s\n' "$MONITOR_IP" | awk -F. 'NF!=4 {exit 1} {for(i=1;i<=4;i++)if($i !~ /^[0-9]+$/ || $i>255)exit 1}' || { echo 'Invalid MONITOR_IP'; exit 1; }
COMMUNITY=${COMMUNITY:-mos2-$(od -An -N12 -tx1 /dev/urandom | tr -d ' \n')}
case "$COMMUNITY" in ''|*[!A-Za-z0-9_-]*) echo 'Use letters, digits, _ or - for COMMUNITY'; exit 1 ;; esac
if command -v opkg >/dev/null 2>&1; then
    opkg update
    opkg install snmpd
elif command -v apk >/dev/null 2>&1; then
    apk update
    apk add snmpd
else
    echo 'Unsupported package manager'; exit 1
fi
backup=/etc/config/snmpd.mos2-backup
[ -f "$backup" ] || cp /etc/config/snmpd "$backup"
cat > /etc/config/snmpd <<EOF
config agent 'mos2'
    option agentaddress 'UDP:161'
config com2sec 'mos2'
    option secname 'mos2'
    option source '$MONITOR_IP/32'
    option community '$COMMUNITY'
config group 'mos2'
    option group 'mos2'
    option version 'v2c'
    option secname 'mos2'
config view 'mos2'
    option viewname 'mos2'
    option type 'included'
    option oid '.1'
config access 'mos2'
    option group 'mos2'
    option context 'none'
    option version 'v2c'
    option level 'noauth'
    option prefix 'exact'
    option read 'mos2'
    option write 'none'
    option notify 'none'
EOF
uci -q delete firewall.mos2_snmp || true
uci set firewall.mos2_snmp=rule
uci set firewall.mos2_snmp.name='MOS2-SNMP'
uci set firewall.mos2_snmp.src='lan'
uci set firewall.mos2_snmp.src_ip="$MONITOR_IP"
uci set firewall.mos2_snmp.dest_port='161'
uci set firewall.mos2_snmp.proto='udp'
uci set firewall.mos2_snmp.target='ACCEPT'
uci commit firewall
/etc/init.d/firewall reload
/etc/init.d/snmpd enable
/etc/init.d/snmpd restart
printf 'SNMP v2c ready. Port: 161; Community: %s; allowed MOS2: %s\n' "$COMMUNITY" "$MONITOR_IP"
