#!/bin/sh
# Existing kernel sensors only: no drivers, daemons or synthetic temperatures.
# Prefer package/Tdie/Tctl. If only per-core readings exist, use the hottest core.
best=
package=
accept() {
    case "$1" in ''|*[!0-9]*) return ;; esac
    [ "$1" -le 150000 ] || return
    [ -n "$best" ] && [ "$best" -ge "$1" ] || best=$1
    case "$2" in Package*|Tdie|Tctl)
        [ -n "$package" ] && [ "$package" -ge "$1" ] || package=$1 ;;
    esac
}
for sensor in /sys/class/hwmon/hwmon*; do
    [ -r "$sensor/name" ] || continue
    case "$(cat "$sensor/name")" in
        coretemp|k10temp|zenpower|soc_dts*)
            for input in "$sensor"/temp*_input; do
                [ -r "$input" ] || continue
                label=${input%_input}_label
                text=
                [ ! -r "$label" ] || text=$(cat "$label")
                accept "$(cat "$input")" "$text"
            done ;;
    esac
done
if [ -n "$package" ]; then printf '%s\n' "$package"; exit 0; fi
if [ -n "$best" ]; then printf '%s\n' "$best"; exit 0; fi
for zone in /sys/class/thermal/thermal_zone*; do
    [ -r "$zone/type" ] && [ -r "$zone/temp" ] || continue
    case "$(cat "$zone/type")" in
        x86_pkg_temp|cpu-thermal|cpu_thermal|soc-thermal|soc_dts*)
            accept "$(cat "$zone/temp")" '' ;;
    esac
done
[ -n "$best" ] || exit 1
printf '%s\n' "$best"
