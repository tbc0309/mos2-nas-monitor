#!/bin/sh
# Ubuntu / Debian-based fnOS: a read-only SNMP agent scoped to one MOS2 address.
set -eu
[ "$(id -u)" = 0 ] || { echo 'Run as root (sudo).'; exit 1; }
: "${MONITOR_IP:?Set MONITOR_IP to the MOS2 IPv4 address}"
printf '%s\n' "$MONITOR_IP" | awk -F. 'NF!=4 {exit 1} {for(i=1;i<=4;i++)if($i !~ /^[0-9]+$/ || $i>255)exit 1}' || { echo 'Invalid MONITOR_IP'; exit 1; }
COMMUNITY=${COMMUNITY:-mos2-$(od -An -N12 -tx1 /dev/urandom | tr -d ' \n')}
case "$COMMUNITY" in ''|*[!A-Za-z0-9_-]*) echo 'Use letters, digits, _ or - for COMMUNITY'; exit 1 ;; esac
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y snmpd snmp
backup=/etc/snmp/snmpd.conf.mos2-backup
[ -f "$backup" ] || cp /etc/snmp/snmpd.conf "$backup"
cat > /etc/snmp/snmpd.conf <<EOF
agentaddress udp:161
view mos2 included .1.3.6.1.2.1
view mos2 included .1.3.6.1.4.1.2021
view mos2 included .1.3.6.1.4.1.8072
rocommunity $COMMUNITY $MONITOR_IP/32 -V mos2
EOF
chmod 600 /etc/snmp/snmpd.conf
systemctl enable snmpd
systemctl restart snmpd
if command -v ufw >/dev/null 2>&1 && ufw status | grep -q '^Status: active'; then
    ufw allow from "$MONITOR_IP" to any port 161 proto udp
fi
printf 'SNMP v2c ready. Port: 161; Community: %s; allowed MOS2: %s\n' "$COMMUNITY" "$MONITOR_IP"
