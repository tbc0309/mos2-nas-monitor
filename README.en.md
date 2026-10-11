# MOS2-NAS

[简体中文](README.md) · **English**

**Version 1.1.1** · Desktop display and Synology NAS monitor for MOS2 · **MIT License**

Monitor CPU, memory, network traffic, system temperature, disk health and storage volumes through SNMP v2c / v3. No Netdata installation is required. Includes a weather clock, photo album, buzzer alarms, QR Wi-Fi setup and bilingual web management.

Hardware: **MOS2 · ESP8266 / ESP-12F · 4MB Flash · ST7789 240×240**.

[Download firmware and simulator](https://github.com/tbc0309/mos2-nas-monitor/releases/latest)

## Interface previews

### NAS monitoring

Six pages, rendered by the shared firmware drawing code using sample data.

![Six NAS pages](docs/images/en/nas-pages.png)

Green represents traffic received by the NAS; red represents traffic sent. Downloads occupy the upper half of the chart and uploads the lower half. Each trend scales independently; use the numeric values to compare actual rates.

### Clock and weather

The original large clock digits and weather animation are retained. Hours default to white and minutes to orange, with separate color controls. Enabled alarms show an icon and time.

<table width="100%">
<tr><th width="50%">Clock with alarm</th><th width="50%">Weather and time</th></tr>
<tr><td align="center"><img src="docs/images/en/desktop-clock.png" width="360" height="360" alt="Clock"></td><td align="center"><img src="docs/images/en/weather-clock.png" width="360" height="360" alt="Weather"></td></tr>
</table>

### Web management

Configure the display, weather, alarms, NAS, Wi-Fi and album in a browser. Switch the web language in the upper right; display language is configured separately.

![Management settings](docs/images/en/web-management.png)

## Quick start

### Install

Download `MOS2-NAS-1.1.1.bin` from [Releases](https://github.com/tbc0309/mos2-nas-monitor/releases/latest). One file contains the application and font resources and supports both USB installation and web updates.

For a fresh installation, replace `COM7` with your serial port:

```shell
python -m pip install esptool
python -m esptool --port COM7 erase-flash
python -m esptool --port COM7 --baud 460800 write-flash --flash-mode dio --flash-size 4MB 0x0 MOS2-NAS-1.1.1.bin
```

A fresh USB installation clears settings and photos. Use the web updater to preserve existing user data.

### Wi-Fi and login

1. On first boot, scan the display QR code to join `MOS2-NAS-xxxxxx`. The hotspot password is `12345678`.
2. Open `http://192.168.4.1/`, scan for a 2.4GHz network, select it and save its password. Hidden SSIDs can be entered manually.
3. After connection, tap five times to display the management QR code.
4. The default management password is `12345678`. First login requires changing it and signing in again.

With saved settings, the device connects automatically. After about 30 seconds of failed connection it opens the setup hotspot. Management sessions expire after 30 minutes of inactivity or a reboot.

### Connect a Synology NAS

Enable SNMP in DSM under **Control Panel → Terminal & SNMP → SNMP**, then fill in the NAS settings:

| Mode | Required settings |
| --- | --- |
| SNMP v2c | NAS address, community and port |
| SNMP v3 | NAS address, username, authentication protocol and password; optional privacy protocol and password |

SNMPv3 supports **MD5 / SHA** authentication and **DES / AES** privacy. The default port is `161`. Network interface selection defaults to automatic; discovered interfaces can be selected later. Leave password fields blank to retain their current values.

Management uses local HTTP. Use it on a trusted network. Additional configuration notes: [SNMP documentation (Chinese)](docs/SNMPV3.md).

## Touch controls

Desktop pages: **Clock → Weather → Album**.

NAS pages: **Overview → Network → CPU / Memory → Disk health → Volumes → Device status**.

| Consecutive taps | Action |
| --- | --- |
| 1 | Next page; return from management or setup display |
| 2 | Next photo or next disk/volume group |
| 3 | Switch desktop / NAS mode |
| 5 | Management QR code |
| 6 | Wi-Fi setup hotspot and QR code |
| 8 | Reset confirmation; a separate triple tap confirms |

Keep gaps between taps below approximately 350ms. Recognition completes after the last release. Four taps, seven taps and long presses have no assigned action. Reset confirmation cancels after ten seconds or another tap sequence.

While an alarm is ringing or snoozed, **one tap snoozes and three taps stop that day's alarm**, taking priority over navigation.

## Configuration

| Section | Settings |
| --- | --- |
| Desktop & display | Display language, startup page, clock colors, seconds, brightness, rotation, touch sound |
| Weather & time | City code with lookup link, refresh interval, NTP server |
| Alarm | Time, weekdays, four 8-bit melodies, snooze duration |
| Photo playback | Slideshow or fixed photo, display duration |
| NAS | Background color, SNMP settings, interface, refresh interval |
| Wi-Fi / admin password | Scan networks, view connection, change credentials |
| Album | Upload, crop, zoom, rotate, add or replace a slot |
| Maintenance | Firmware update, data retention, factory reset |

Weather defaults to Tianjin (`101030100`), with a 5–60 minute refresh interval. NTP defaults to `ntp.aliyun.com`, timezone UTC+8. Scheduled alarms require synchronized time. Alarms default to weekdays; snooze supports 1–30 minutes.

The album holds **up to seven 240×240 JPEG photos, at most 48KB each**. Double tap to change photos in fixed mode.

### Monitoring behavior

- CPU usage uses cumulative counter differences, excluding idle and I/O wait.
- Memory usage excludes free memory, buffers and cache; sample timing may differ from DSM.
- Network rates use IF-MIB Counter64 differences, in bytes per second.
- The temperature ring is green below 60°C, orange at 60–74°C and red from 75°C. These are display thresholds, not DSM alarm settings.
- Up to 12 disks and eight `/volumeN` volumes are supported, grouped into four disks or three volumes per page.
- Data groups are collected and cached independently. Unknown data is displayed as unknown.
- Startup prioritizes the selected page. Disk health refreshes hourly, disk temperature every five minutes and volume usage every two hours. NAS uptime advances locally after the first reading and synchronizes every two hours.
- Global request spacing, a single pending request and failure backoff limit NAS load.

## Update and reset

Under **Maintenance → Firmware update**, select `MOS2-NAS-1.1.1.bin`. **Keep settings and photos** is enabled by default. Unchecking it clears user data only after a validated, successful update; validation failure does not clear data.

![Update and reset](docs/images/en/web-maintenance.png)

The updater validates the full file, extracts the application and retains compatible system resources. Use USB installation if an older updater cannot recognize the unified file or the font resources are incompatible.

Factory reset removes settings, the management password and all photos while retaining system fonts. Wi-Fi setup is required after reboot.

## Simulator

Download the Windows simulator archive from [Releases](https://github.com/tbc0309/mos2-nas-monitor/releases/latest), extract it completely and launch `MOS2-NAS-Simulator.exe`. The browser opens automatically.

The simulator displays the screen and management interface and can query a NAS reachable from the computer. It runs shared rendering code, not the ESP8266 binary. Replacing a bin file does not change its simulated interface. Buzzer sound, SPI refresh and actual Wi-Fi provisioning require hardware testing.

To run from source, install Python dependencies and a C++17 compiler:

```shell
python -m pip install -r simulator/requirements.txt
g++ -std=c++17 -O2 -static -Iinclude -Isimulator/native simulator/native/renderer.cpp -o simulator/renderer.exe
python simulator/server.py
```

Open `http://127.0.0.1:8765/` for the display and `/manage` for settings. Rebuild the renderer after drawing-code changes. The server listens only on loopback.

## Development

Generated fonts and web headers are included; a regular build does not require installing the Tencent font.

```shell
python -m pip install platformio
pio run
pio run -t buildfs
python tools/build_install_image.py --output MOS2-NAS-1.1.1.bin
```

`src/main.cpp` contains startup and the main loop. `src/app/` separates display, collection, configuration, web, touch and update logic. `include/` contains protocol and rendering components.

Web sources are in `web/`; language packs are in `locales/` and `include/locales/`. Regenerate after changes:

```shell
python tools/embed_locales.py
python tools/embed_web.py
```

The display uses Tencent W7 native hinting with 16 grayscale levels and on-demand font reads. The large clock retains the original image digits. Font regeneration requires a font file you are licensed to use:

```powershell
python -m pip install -r tools/font-requirements.txt
$env:FONT_PATH = 'C:/Fonts/TencentSans-W7.ttf'
python tools/build_fonts.py
```

After installing simulator dependencies, a C++17 compiler and Node.js, and building the firmware, run `python tools/check.py` for component and simulator checks.

Further technical documentation (Chinese): [Code structure](docs/STRUCTURE.md) · [Data collection](docs/COLLECTION.md) · [Filesystem and updates](docs/FILESYSTEM.md).

### Hardware pins

| Function | GPIO |
| --- | --- |
| TFT SCK / MOSI | 14 / 13 |
| TFT DC / RESET | 0 / 2 |
| TFT CS | Ground, configured as -1 |
| Backlight | 5, active low |
| TTP223 touch | 4, active low |
| Buzzer | 12 |

The board uses a CH340 USB serial adapter. Follow the original MOS2 power and logic-level design.

## License and credits

Original project code is licensed under **MIT**. Third-party dependencies, fonts and reference assets retain their respective licenses; see [NOTICE](NOTICE.md).

Hardware, large clock digits, weather icons and animation reference JSRUN MOS2 / SmallDesktopDisplay. The NAS interface references routermonitor.

- [MOS2 official](https://jsrun.net/hw/mos2)
- [imnks.com](https://imnks.com/)
