"""Local MOS2 screen simulator, using the firmware's C++ dashboard renderer."""

from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from pathlib import Path
import argparse, copy, json, math, subprocess, sys, threading, time, re, os
from datetime import datetime, timezone, timedelta
from urllib.parse import parse_qs, urlencode, urlparse
from urllib.request import urlopen
import urllib.request, urllib.error
import ipaddress, secrets, io
from http.cookies import SimpleCookie
import html
from PIL import Image

ROOT = (
    Path(sys._MEIPASS)
    if getattr(sys, "frozen", False)
    else Path(__file__).resolve().parents[1]
)
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "simulator"))
from security import LoginGuard, local_request
from credentials import password_valid, default_password
from configuration import read_config, write_atomic, normalize_config
from snmp_profiles import Profile, profiles
CITY_CODES = {row[2] for row in json.loads((ROOT / "web/weather-cities.js").read_text(encoding="utf8").split("=",1)[1].strip().rstrip(";"))}
from snmp_probe import query as snmp_query
from snmp_v3 import Credentials, query as snmp_v3_query, clear_sessions

# FIFO send budget prevents a long table walk from repeatedly reacquiring a
# semaphore ahead of another collector. Only reply waiting occupies a slot.
_snmp_condition = threading.Condition()
_snmp_sent_at = 0.0
_snmp_active = 0
_snmp_ticket = 0
_snmp_serving = 0
_snmp_failures = 0
_snmp_cooldown = 0.0
collecting = False


def query(host, port, community, name, next_oid=False):
    global _snmp_sent_at, _snmp_active, _snmp_ticket, _snmp_serving, _snmp_failures, _snmp_cooldown
    with _snmp_condition:
        ticket = _snmp_ticket
        _snmp_ticket += 1
        while True:
            if ticket == _snmp_serving and collecting and (host, port, community) != target_key(config):
                _snmp_serving += 1
                _snmp_condition.notify_all()
                raise OSError("Collector target changed")
            wait = max(
                0.5 - (time.monotonic() - _snmp_sent_at),
                _snmp_cooldown - time.monotonic(),
            )
            if ticket == _snmp_serving and _snmp_active < 1 and wait <= 0:
                _snmp_active += 1
                _snmp_serving += 1
                _snmp_sent_at = time.monotonic()
                _snmp_condition.notify_all()
                break
            _snmp_condition.wait(max(0.001, min(0.075, wait)) if wait > 0 else 0.075)
    try:
        if collecting and (host, port, community) != target_key(config):
            raise OSError("Collector target changed")
        if isinstance(community, Credentials):
            result = snmp_v3_query(host, port, community, name, next_oid, timeout=4)
        else:
            result = snmp_query(host, port, community, name, next_oid, timeout=4)
        with _snmp_condition:
            _snmp_failures = 0
            _snmp_cooldown = 0.0
        return result
    except TimeoutError:
        with _snmp_condition:
            if not collecting or (host, port, community) == target_key(config):
                _snmp_failures += 1
                if _snmp_failures >= 3:
                    _snmp_cooldown = time.monotonic() + 60
        raise
    finally:
        with _snmp_condition:
            # Start spacing after transport completion: thread scheduling or
            # first-session key derivation must not compress actual send gaps.
            _snmp_sent_at = time.monotonic()
            _snmp_active -= 1
            _snmp_condition.notify_all()


config = dict(
    machines=[],
    activeMachine=0,
    machineName="",
    nas="192.168.1.100",
    community="public",
    snmpVersion="2c",
    snmpUser="",
    snmpPassword="",
    snmpAuth="MD5",
    snmpPrivacy="none",
    snmpPrivacyPassword="",
    port=161,
    mode="snmp",
    nasBackground="#21455c",
    cardBackground="#103040",
    language="zh",
    adminLanguage="zh",
    clockColor="#ffffff",
    minuteColor="#ffa500",
    albumFixed=0,
    showSeconds=1,
    alarmDays=31,
    alarmTune=0,
    sound=1,
    alarmEnabled=0,
    alarmTime="07:00",
    snooze=5,
    albumInterval=10,
    weatherEnabled=1,
    cityCode="101030100",
    weatherInterval=20,
    rotation=0,
    ntpServer="ntp.aliyun.com",
    storage="",
    iface="",
    interval=5,
    brightness=70,
    home=6,
    ssid="",
    **{"pass": "", "admin": "12345678", "adminDefault": True},
)
state = dict(
    cpu=-1,
    memory=-1,
    storage=-1,
    temperature=-1,
    rx=-1,
    tx=-1,
    model="",
    volume="",
    interface="",
    online=False,
    page=6,
    mode="snmp",
    memoryTotal=0,
    memoryAvail=0,
    volumeTotal=0,
    volumeUsed=0,
    uptime=0,
    age=0,
    ip="127.0.0.1",
    rssi=-45,
    history=[],
    diskGroup=0,
    volumeGroup=0,
)
defaults = copy.deepcopy(config)
defaults.update(nas="192.168.1.100", community="public", storage="", iface="")
ui_mode = ""
reset_pending = False
reset_started = 0
reset_count = 0
reset_tap_at = 0
tap_count = 0
tap_at = 0
reset_nonce = ""
reset_nonce_at = 0
lock = threading.Lock()
token = secrets.token_hex(16)
sessions = {}
login_guard = LoginGuard()
revision = 0
last_ok = 0
weather = dict(
    enabled=True,
    valid=False,
    city="",
    condition="",
    temperature=-999,
    humidity=-1,
    aqi=-1,
    observed="",
    error="Waiting for weather",
    age=0,
    cityEnglish="",
    conditionEnglish="",
    windEnglish="",
    code=-1,
)
weather_at = 0
album_dir = (
    Path(sys.executable).parent / "data"
    if getattr(sys, "frozen", False)
    else ROOT / "simulator/data"
)
album_dir.mkdir(parents=True, exist_ok=True)
# Source tests can opt into a private state path; packaged runs use their writable data folder.
config_path = None


MACHINE_FIELDS = (
    "cardBackground",
    "nasBackground",
    "nas",
    "community",
    "snmpVersion",
    "snmpUser",
    "snmpPassword",
    "snmpAuth",
    "snmpPrivacy",
    "snmpPrivacyPassword",
    "port",
    "iface",
    "storage",
    "interval",
)


weather_validation = dict(code="", status="idle", completed=0)

def weather_code_ready(code):
    return code in CITY_CODES or code == config["cityCode"] or (
        code == weather_validation["code"] and weather_validation["status"] == "valid"
        and time.monotonic()-weather_validation["completed"] < 600)

def probe_weather_code(code):
    status = "unavailable"
    try:
        req=urllib.request.Request(f"http://d1.weather.com.cn/weather_index/{code}.html",
              headers={"Referer":"http://www.weather.com.cn/", "User-Agent":"Mozilla/5.0"})
        text=urllib.request.urlopen(req,timeout=8).read(65536).decode("utf8")
        match=re.search(r"var dataSK\s*=\s*(\{.*?\})\s*;",text)
        if match:
            data=json.loads(match[1]); t=float(data["temp"])
            status="valid" if data.get("city")==code and data.get("cityname") and data.get("weather") and math.isfinite(t) and -80<=t<=65 else "invalid"
        elif re.search(r"var fc\s*=",text): status="forecast"
        else: status="invalid"
    except urllib.error.HTTPError as error:
        status="invalid" if error.code==404 else "unavailable"
    except (OSError,ValueError,KeyError,TypeError): pass
    with lock:
        if weather_validation["code"]==code:
            weather_validation.update(status=status,completed=time.monotonic())

def weather_english(code):
    mapping = {0:"Sunny",1:"Cloudy",2:"Overcast",3:"Showers",4:"Thunderstorms",5:"Thunderstorms with hail",6:"Sleet",7:"Light rain",8:"Rain",9:"Heavy rain",13:"Snow showers",14:"Light snow",15:"Snow",16:"Heavy snow",17:"Blizzard",18:"Fog",19:"Freezing rain",20:"Sandstorm",29:"Dust",30:"Blowing sand",31:"Sandstorm",32:"Dense fog",49:"Dense fog",301:"Rain",302:"Snow"}
    if 10 <= code <= 12: return "Rainstorm"
    if 21 <= code <= 25: return "Rain"
    if 26 <= code <= 28: return "Snow"
    if 53 <= code <= 56: return "Haze"
    return mapping.get(code,"Weather")


def capture_machine():
    if not config["machines"]:
        config["machines"] = [{}]
        config["activeMachine"] = 0
    config["machines"][config["activeMachine"]] = {
        **{k: config[k] for k in MACHINE_FIELDS},
        "name": config["machineName"],
    }


selection_save_at = 0.0


def select_machine(index, persist=True):
    global ui_mode, revision, selection_save_at
    capture_machine()
    config["activeMachine"] = index
    m = config["machines"][index]
    config.update({k: m[k] for k in MACHINE_FIELDS})
    config["machineName"] = m["name"]
    if persist:
        persist_config()  # Management edits remain transactional.
        selection_save_at = 0.0
    else:
        selection_save_at = time.monotonic() + 5.0
    clear_telemetry()
    # Desktop pages enter overview; existing monitor pages retain their position.
    if state["page"] not in (0, 1, 2, 4, 8, 9):
        state["page"] = 0
    # Preserve the current page when changing devices.
    state["machineNotice"] = m["name"] or m["nas"]
    state["machineNoticeUntil"] = time.monotonic() + 1.8
    ui_mode = ""
    revision += 1


def machine_action(form):
    """Apply a profile change atomically; caller holds the application lock."""
    before = copy.deepcopy(config)
    try:
        capture_machine()
        action = form.get("action")
        if action == "add":
            if len(config["machines"]) >= 6:
                raise ValueError("Profile limit")
            config["machines"].append(
                {
                    **{k: copy.deepcopy(defaults[k]) for k in MACHINE_FIELDS},
                    "name": "",
                }
            )
            index = len(config["machines"]) - 1
        elif action == "move":
            if not re.fullmatch(r"[0-5]:[0-5]", form.get("index", "")):
                raise ValueError("Order")
            try:
                source, destination = map(int, form.get("index", "").split(":"))
            except ValueError:
                raise ValueError("Order")
            machines = config["machines"]
            if not (
                0 <= source < len(machines)
                and 0 <= destination < len(machines)
                and abs(source - destination) == 1
            ):
                raise ValueError("Order")
            machines[source], machines[destination] = (
                machines[destination],
                machines[source],
            )
            active = config["activeMachine"]
            config["activeMachine"] = (
                destination
                if active == source
                else source if active == destination else active
            )
            persist_config()
            return config["activeMachine"]
        elif action == "delete":
            machines = config["machines"]
            if len(machines) <= 1:
                raise ValueError("Keep one device")
            removed = int(form.get("index", config["activeMachine"]))
            if not 0 <= removed < len(machines): raise ValueError("Index")
            active = config["activeMachine"]
            machines.pop(removed)
            config["activeMachine"] = 0 if active == removed else active - (active > removed)
            current = machines[config["activeMachine"]]
            config.update({k: current[k] for k in MACHINE_FIELDS})
            config["machineName"] = current["name"]
            persist_config()
            if removed == active: clear_telemetry()
            return config["activeMachine"]
        elif action == "activate":
            index = (config["activeMachine"] + 1) % len(config["machines"])
            select_machine(index, persist=False)
            return index
        elif action == "select":
            value = form.get("index", "")
            if not re.fullmatch(r"[0-5]", value) or int(value) >= len(
                config["machines"]
            ):
                raise ValueError("Index")
            index = int(value)
            return index
        else:
            raise ValueError("Action")
        persist_config()
        return index
    except Exception:
        config.clear()
        config.update(before)
        raise


def clear_telemetry():
    """Invalidate target-owned data synchronously, before HTTP returns to the UI."""
    global last_ok, collection_target, reset_nonce, _snmp_failures, _snmp_cooldown
    with _snmp_condition:
        _snmp_failures = 0
        _snmp_cooldown = 0.0
        _snmp_condition.notify_all()
    profiles.clear()
    last_ok = 0
    reset_nonce = ""
    collection_target = target_key(config)
    collection_stamps.clear()
    collection_diagnostics.clear()
    table_stamps.clear()
    interface_cache.clear()
    interface_scan.clear()
    profiles.clear()
    clear_sessions()
    disks.clear()
    volumes.clear()
    state.update(
        cpu=-1,
        memory=-1,
        temperature=-1,
        rx=-1,
        tx=-1,
        model="",
        history=[],
        uptime=0,
        storage=-1,
        online=False,
        interface="",
        interfaces=[],
        memoryTotal=0,
        memoryAvail=0,
        volumeTotal=0,
        volumeUsed=0,
        volume="",
        diskGroup=0,
        volumeGroup=0,
    )


def persist_config():
    if config_path is None:
        return
    write_atomic(config_path, config)


def load_config(path):
    global config_path
    config_path = Path(path)
    if config_path.is_file():
        try:
            saved = read_config(config_path)
            config.update(normalize_config(saved, defaults, MACHINE_FIELDS))
        except (ValueError, OSError):
            print("Invalid configuration file preserved; using defaults.", file=sys.stderr)
            config.clear(); config.update(copy.deepcopy(defaults))
    config_path.with_suffix(".tmp").unlink(missing_ok=True)
    (album_dir / "incoming.jpg").unlink(missing_ok=True)
    capture_machine()
    state["page"] = config["home"]


alarm = dict(enabled=0, time="07:00", state=0, remaining=0)
alarm_due = 0
alarm_start = 0
alarm_snooze_used = False
alarm_day = ""
alarm_taps = 0
alarm_tap_at = 0
alarm_manual = False
disks = []
volumes = []
album_position = 0
album_at = time.monotonic()


def erase_album():
    global alarm_due, alarm_start, alarm_day, alarm_taps, alarm_tap_at, alarm_manual, album_position
    alarm_due = alarm_start = alarm_taps = alarm_tap_at = 0
    alarm_day = ""
    alarm_manual = False
    album_position = 0
    alarm.update(enabled=0, state=0, remaining=0, time="07:00")
    clear_telemetry()
    state.update(
        online=False,
        cpu=-1,
        memory=-1,
        storage=-1,
        temperature=-1,
        rx=-1,
        tx=-1,
        diskGroup=0,
        volumeGroup=0,
    )
    for p in album_dir.glob("photo-*"):
        if p.is_file():
            p.unlink()


def album_entries():
    return [
        dict(
            slot=i,
            url=f'/api/album/image?slot={i}&v={(album_dir/f"photo-{i}.jpg").stat().st_mtime_ns}',
        )
        for i in range(7)
        if (album_dir / f"photo-{i}.jpg").exists()
    ]


def next_page():
    group = (
        ([6, 5, 7] if config["weatherEnabled"] else [6, 7])
        if state["page"] in (5, 6, 7)
        else [0, 1, 2, 8, 9, 4]
    )
    state["page"] = group[(group.index(state["page"]) + 1) % len(group)]
    state["diskGroup"] = state["volumeGroup"] = 0


def alarm_tap():
    global alarm_due, alarm_taps, alarm_tap_at, alarm_manual, alarm_snooze_used
    now = time.monotonic()
    alarm_taps = alarm_taps + 1 if now - alarm_tap_at <= 0.8 else 1
    alarm_tap_at = now
    if alarm_taps >= 3:
        alarm.update(state=3, remaining=0)
        alarm_manual = False
        alarm_taps = 0
    elif alarm["state"] == 1 and alarm_snooze_used:
        alarm.update(state=3, remaining=0)
        alarm_manual = False
    elif alarm["state"] == 1:
        alarm_snooze_used = True
        alarm["state"] = 2
        alarm_due = time.time() + config["snooze"] * 60
        alarm["remaining"] = config["snooze"] * 60


def alarm_poll():
    global alarm_due, alarm_day, alarm_start, alarm_taps, alarm_snooze_used, alarm_manual
    while True:
        with lock:
            dt = datetime.now(timezone(timedelta(hours=8)))
            now = time.time()
            alarm.update(enabled=config["alarmEnabled"], time=config["alarmTime"])
            if not config["alarmEnabled"] and not alarm_manual:
                alarm["state"] = 0
            elif (
                not alarm_manual
                and dt.strftime("%H:%M") == config["alarmTime"]
                and alarm_day != dt.strftime("%Y%m%d")
                and config["alarmDays"] & (1 << dt.weekday())
            ):
                alarm_day = dt.strftime("%Y%m%d")
                alarm.update(state=1)
                alarm_start = now
                alarm_snooze_used = False
                state["page"] = 6
                alarm_taps = 0
            if alarm["state"] == 2 and now >= alarm_due:
                alarm["state"] = 1
                alarm_start = now
                alarm_taps = 0
            if alarm["state"] == 1 and now - alarm_start >= 300:
                alarm["state"] = 3
                alarm_manual = False
            alarm["remaining"] = (
                max(0, int(alarm_due - now)) if alarm["state"] == 2 else 0
            )
        time.sleep(0.1)


table_stamps = {}
storage_discovery_generation = 0
storage_discovery_requested_at = 0.0


def table_poll(kind):
    global disks, volumes
    seen, metadata_at, due = None, 0, 0
    discovery_generation = storage_discovery_generation
    scan_cursor, scan_found = None, []
    while True:
        if not activity_allowed("snmp"):
            time.sleep(0.2)
            continue
        with lock:
            cfg = copy.deepcopy(config)
            rows = copy.deepcopy(disks if kind == "disk" else volumes)
        key = target_key(cfg)
        now = time.monotonic()
        if seen != key:
            seen, metadata_at, due, rows = key, 0, 0, []
            scan_cursor, scan_found = None, []
        if discovery_generation != storage_discovery_generation:
            discovery_generation = storage_discovery_generation
            metadata_at = due = 0
            scan_cursor, scan_found = None, []
        if now < due or not startup_ready(cfg, kind):
            time.sleep(0.1)
            continue

        def sn(oid, next_oid=False):
            return query(*target_key(cfg), oid, next_oid)

        if kind == "disk" and not profiles.get(key, Profile()).synology_confirmed:
            # Generic inventory is separate from SMART; never invent health.
            found = []
            try:
                for root, generic in (("1.3.6.1.2.1.25.3.6.1.2", True), ("1.3.6.1.4.1.2021.13.15.1.1.2", False)):
                    cursor = root
                    for _ in range(64):
                        try:
                            oid, value = sn(cursor, True)
                        except ValueError:
                            break
                        if not oid.startswith(root + ".") or len(found) >= 12:
                            break
                        cursor = oid
                        if not generic and not re.fullmatch(r"(?:sd|hd|vd|xvd)[a-z]+|nvme[0-9]+n[0-9]+|mmcblk[0-9]+|sata[0-9]+", str(value)):
                            continue
                        index = oid.rsplit(".", 1)[1]
                        row = dict(index=index, name=str(value) if not generic else "Disk " + index,
                                   model="--", status=-1, health=-1, temperature=-1)
                        if generic:
                            try:
                                desc = sn("1.3.6.1.2.1.25.3.2.1.3." + index)[1]
                                row["model"] = str(desc)[:64]
                                match = re.search(r"/dev/([^ )]+)", str(desc))
                                if match: row["name"] = match.group(1)
                                if (match and not re.fullmatch(r"(?:sd|hd|vd|xvd)[a-z]+|nvme[0-9]+n[0-9]+|mmcblk[0-9]+|sata[0-9]+", row["name"])) or str(desc).startswith(("RAID ", "LVM ")):
                                    continue
                            except ValueError: pass
                            try:
                                size = sn("1.3.6.1.2.1.25.3.6.1.4." + index)[1]
                                if isinstance(size, int) and size > 0: row["model"] = f"{size / 1048576:.1f} GiB"
                            except ValueError: pass
                        found.append(row)
                    if found: break
                with lock:
                    if target_key(config) == key: disks = copy.deepcopy(found)
                metadata_at = time.monotonic()
                due = metadata_at + 7200
            except OSError:
                due = time.monotonic() + 120
            continue

        ok = True
        if (
            scan_cursor is not None
            or not rows
            or not metadata_at
            or now - metadata_at >= 7200
        ):
            try:
                root = (
                    "1.3.6.1.4.1.6574.2.1.1.2"
                    if kind == "disk"
                    else "1.3.6.1.2.1.25.2.3.1.3"
                )
                cursor, found = scan_cursor or root, copy.deepcopy(scan_found)
                found = [
                    next(
                        (copy.deepcopy(r) for r in rows if r["index"] == v["index"]), v
                    )
                    for v in found
                ]
                for _ in range(128):
                    oid, name = sn(cursor, True)
                    if not oid.startswith(root + "."):
                        break
                    if kind == "volume":
                        if (
                            ("/@" in str(name) and not re.search(r"/@usb(?:/|$)",str(name)))
                            or str(name).startswith(
                                ("/var/lib/docker", "/var/lib/containers")
                            )
                            or (
                                name == "/"
                                and profiles.get(key, Profile()).synology_confirmed
                            )
                            or re.match(
                                r"^/(?:dev|proc|sys|boot|rom|snap)(?:/|$)|^/run(?:$|/(?!media/))|^/tmp(?:$|/(?!mnt/))",
                                str(name),
                            )
                        ):
                            cursor = oid
                            continue
                        if name == "/" and any(
                            v.get("name") == "/overlay" for v in found
                        ):
                            cursor = oid
                            continue
                        if profiles.get(key, Profile()).synology_confirmed and not re.fullmatch(r'/volume[0-9]+|/volumeUSB[0-9]+(?:/usbshare[^/]*)?',str(name)):
                            cursor = oid
                            continue
                        try:
                            storage_type = sn(
                                "1.3.6.1.2.1.25.2.3.1.2." + oid.rsplit(".", 1)[1]
                            )[1]
                            accepted = storage_type in (
                                "1.3.6.1.2.1.25.2.1.4",
                                "1.3.6.1.2.1.25.2.1.5",
                                "1.3.6.1.2.1.25.2.1.9",
                            )
                        except ValueError:
                            accepted = bool(
                                re.fullmatch(
                                    r"(?:/volume[0-9]+|/volumeUSB[0-9]+(?:/.*)?|/vol(?!ume)[^/]*(?:/.*)?|/www[^/]*(?:/.*)?|/(?:mnt|media|run/media|tmp/mnt)/.+|/)",
                                    str(name),
                                )
                            )
                        if not accepted:
                            cursor = oid
                            continue
                    index = oid.rsplit(".", 1)[1]
                    row = next(
                        (copy.deepcopy(r) for r in rows if r["index"] == index),
                        dict(index=index),
                    )
                    row["name"] = name
                    if kind == "disk":
                        row.setdefault("model", "")
                        for field in ("status", "temperature", "health"):
                            row.setdefault(field, -1)
                    else:
                        units = sn("1.3.6.1.2.1.25.2.3.1.4." + index)[1]
                        size = sn("1.3.6.1.2.1.25.2.3.1.5." + index)[1]
                        if not isinstance(units,int) or not isinstance(size,int) or units <= 0 or size <= 0 or size > (2**64-1)//units:
                            raise ValueError("Invalid volume capacity")
                        row.update(units=units, total=units * size)
                        used = sn("1.3.6.1.2.1.25.2.3.1.6." + index)[1]
                        if not isinstance(used, int) or not 0 <= used <= size:
                            raise ValueError("Invalid volume use")
                        row.update(used=units * used, valid=True)
                        with lock:
                            table_stamps[(kind, index, "used")] = time.monotonic()
                    found.append(row)
                    cursor = oid
                    # Publish discovered capacity without waiting for every table row.
                    with lock:
                        if target_key(config) == key:
                            existing = disks if kind == "disk" else volumes
                            combined = copy.deepcopy(found) + [
                                r
                                for r in existing
                                if r.get("index") not in {v["index"] for v in found}
                            ]
                            if kind == "disk":
                                disks = combined
                            else:
                                volumes = combined
                    if len(found) >= (12 if kind == "disk" else 8):
                        break
                rows, metadata_at = found, time.monotonic()
                with lock:
                    if target_key(config) == key:
                        if kind == "disk":
                            disks = copy.deepcopy(rows)
                        else:
                            volumes = copy.deepcopy(rows)
                scan_cursor, scan_found = None, []
            except (OSError, ValueError, KeyError, TypeError) as error:
                ok = False
                scan_cursor, scan_found = cursor, copy.deepcopy(found)
                if found:
                    rows = found + [
                        r
                        for r in rows
                        if r.get("index") not in {v["index"] for v in found}
                    ]
                print(f"SNMP {kind} discovery: {error}", file=sys.stderr, flush=True)
        for row in rows:
            fields = (
                (("status", 5), ("temperature", 6), ("health", 13))
                if kind == "disk"
                else (("used", 6),)
            )
            for field, column in fields:
                if (
                    kind == "volume"
                    and table_stamps.get((kind, row["index"], field), -math.inf) >= now
                ):
                    continue  # Already acquired while discovering this row.
                if (
                    kind == "disk"
                    and field != "temperature"
                    and now - table_stamps.get((kind, row["index"], field), -math.inf)
                    < 3600
                ):
                    continue
                try:
                    root = (
                        "1.3.6.1.4.1.6574.2.1.1"
                        if kind == "disk"
                        else "1.3.6.1.2.1.25.2.3.1"
                    )
                    value = sn(root + "." + str(column) + "." + row["index"])[1]
                    if not isinstance(value, int) or value < 0:
                        raise ValueError("Invalid counter")
                    if kind == "disk" and (value > (150 if field == "temperature" else 0x7fffffff) or (field != "temperature" and value == 0)):
                        raise ValueError("Invalid disk measurement")
                    if kind == "volume":
                        value *= row["units"]
                        if value > row["total"]:
                            raise ValueError("Invalid volume use")
                        row["valid"] = True
                    row[field] = value
                    with lock:
                        if target_key(config) == key:
                            table_stamps[(kind, row["index"], field)] = time.monotonic()
                except (OSError, ValueError, KeyError, TypeError):
                    ok = False
                with lock:
                    if target_key(config) == key:
                        if kind == "disk":
                            disks = copy.deepcopy(rows)
                        else:
                            volumes = copy.deepcopy(rows)
                        expire_tables(time.monotonic())
        due = time.monotonic() + (
            (300 if kind == "disk" and rows else 7200)
            if ok
            else (15 if not metadata_at else 120)
        )


def expire_tables(now):
    for kind, rows, ttl in (("disk", disks, 7200), ("volume", volumes, 14400)):
        for row in rows:
            for field in (
                ("status", "temperature", "health") if kind == "disk" else ("used",)
            ):
                if now - table_stamps.get(
                    (kind, row.get("index"), field), -math.inf
                ) > (900 if kind == "disk" and field == "temperature" else ttl):
                    row[field] = -1 if kind == "disk" else None
                    if kind == "volume":
                        row["valid"] = False


def extra_poll():
    for kind in ("disk", "volume"):
        threading.Thread(target=table_poll, args=(kind,), daemon=True).start()


def weather_retry(failures):
    return (60, 60, 300, 600, 1200, 1800)[min(max(failures,0),5)]


def weather_poll():
    global weather_at
    seen = -1
    attempt = 0
    failures = 0
    while True:
        with lock:
            cfg = copy.deepcopy(config)
            rev = (cfg["cityCode"], cfg["weatherEnabled"])
            error = weather["error"]
            if seen != -1 and seen != rev:
                failures = 0
                weather.update(valid=False, city="", condition="")
                weather_at = 0
        now = time.monotonic()
        if activity_allowed("weather") and cfg["weatherEnabled"] and (
            rev != seen
            or now - attempt >= (weather_retry(failures) if error else cfg["weatherInterval"] * 60)
        ):
            attempt = now
            seen = rev
            try:
                req = __import__("urllib.request", fromlist=["Request"]).Request(
                    f"http://d1.weather.com.cn/weather_index/{cfg['cityCode']}.html?_={int(time.time())}",
                    headers={
                        "User-Agent": "Mozilla/5.0",
                        "Referer": "http://www.weather.com.cn/",
                    },
                )
                with urlopen(req, timeout=4) as r:
                    body = r.read(8193)
                    if len(body) > 8192:
                        raise ValueError("Response too large")
                text = body.decode("utf8")
                match = re.search(r"var dataSK\s*=\s*(\{.*?\})\s*;", text)
                if not match:
                    raise ValueError("Missing dataSK")
                data = json.loads(match[1])
                t = float(data["temp"])
                h = -1
                try:
                    candidate = float(str(data.get("SD", "")).replace("%", ""))
                    if math.isfinite(candidate) and 0 <= candidate <= 100: h = candidate
                except (ValueError, TypeError): pass
                if data["city"] != cfg["cityCode"] or not (
                    -80 <= t <= 65
                ):
                    raise ValueError("Invalid city or values")
                update = dict(
                    valid=True,
                    city=data["cityname"],
                    condition=data["weather"],
                    temperature=t,
                    humidity=h,
                    aqi=int(data["aqi"]) if data.get("aqi", "").isdigit() else -1,
                    observed=data["time"],
                    error="",
                    cityEnglish=data["nameen"],
                    conditionEnglish=data.get("weathere") or weather_english(int(re.sub(r"^[dn]", "", data.get("weathercode", "-1")))),
                    windEnglish=data["wde"] + " " + data["wse"],
                    wind=data["WD"] + " " + data["WS"],
                    code=int(data["weathercode"][1:]),
                )
                with lock:
                    if rev == (config["cityCode"], config["weatherEnabled"]):
                        if weather.get("cityEnglish") != update["cityEnglish"]:
                            weather["scrollStarted"] = time.monotonic()
                        weather.update(update)
                        weather_at = time.monotonic()
                        failures = 0
            except (OSError, ValueError, KeyError, TypeError) as e:
                with lock:
                    if rev == (config["cityCode"], config["weatherEnabled"]):
                        weather["error"] = str(e)
                        failures = min(failures+1,5)
        time.sleep(0.25)




interface_cache = {}
interface_scan = {}


STARTUP_GROUPS = (
    "cpu",
    "memory",
    "network",
    "temperature",
    "uptime",
    "identity",
    "disk",
    "volume",
    "interfaces",
)
_startup_lock = threading.Lock()
_startup_key, _startup_page, _startup_due = None, None, {}
_storage_released, _startup_at = False, 0.0


def startup_delays(page):
    if page in (5, 6, 7):
        return dict(zip(STARTUP_GROUPS, (20, 21, 23, 22, 25, 30, 35, 40, 22)))
    delay = dict(zip(STARTUP_GROUPS, (30, 31, 32, 33, 45, 0, 65, 70, 30)))
    primary = {
        0: ("cpu", "memory", "network", "temperature", "interfaces"),
        1: ("network", "interfaces"),
        2: ("cpu", "memory"),
        4: ("uptime", "identity", "interfaces"),
        8: ("disk",),
        9: ("volume",),
    }
    for group in primary.get(page, ()):
        delay[group] = 0
    return delay


_activity_page = -1
_activity_entered = 0.0
_management_demand = -1e9

def activity_allowed(kind, now=None):
    """Pause invisible collectors after grace without discarding their snapshots."""
    global _activity_page, _activity_entered
    now = time.monotonic() if now is None else now
    page = state["page"]
    if page != _activity_page:
        _activity_page, _activity_entered = page, now
    if kind == "weather":
        return page == 5 or now - _activity_entered < 300
    return page not in (5, 6, 7) or now - _activity_entered < 300 or now - _management_demand < 10

def startup_ready(cfg, group):
    global _startup_key, _startup_page, _startup_due, _storage_released, _startup_at
    if not activity_allowed("snmp"): return False
    now, page = time.monotonic(), state["page"]
    key = target_key(cfg)
    with _startup_lock:
        if key != _startup_key:
            _startup_key, _startup_page = key, page
            _storage_released, _startup_at = False, now
            _startup_due = {g: now + d for g, d in startup_delays(page).items()}
        elif page != _startup_page:
            _startup_page = page
            for g, d in startup_delays(page).items():
                _startup_due[g] = min(_startup_due[g], now + d)
        if group in ("disk", "volume"):
            # Stamps record successful coherent publication, independent of expiry.
            ready = collection_target == key and all(
                g in collection_stamps
                or getattr(profiles.get(key, Profile()), {"cpu":"cpu_mode","memory":"memory_mode","network":"network_mode","temperature":"temperature_mode"}[g]) == "unsupported"
                for g in ("cpu", "memory", "network", "temperature")
            )
            _storage_released |= ready or page in (8, 9) or now - _startup_at >= 120
            return _storage_released
        return now >= _startup_due[group]


def discover_interfaces(cfg):
    key = target_key(cfg)
    cached = interface_cache.get(key)
    if (
        cached
        and time.monotonic() - cached[0] < 300
        and any(p["up"] for p in cached[1])
    ):
        return cached[1]
    root = "1.3.6.1.2.1.2.2.1.2"
    cursor, entries = interface_scan.get(key, (root, []))
    entries = copy.deepcopy(entries)
    failed = False
    for _ in range(64):
        interface_scan[key] = cursor, copy.deepcopy(entries)
        oid, name = query(*key, cursor, next_oid=True)
        if not oid.startswith(root + "."):
            break
        cursor = oid
        index = oid.rsplit(".", 1)[1]
        try:
            if query(*key, "1.3.6.1.2.1.2.2.1.3." + index)[1] in (6,23,71,161,209):
                entries.append(
                    dict(
                        index=index,
                        name=name,
                        up=query(*key, "1.3.6.1.2.1.2.2.1.8." + index)[1] == 1,
                    )
                )
        except (OSError, ValueError):
            failed = True
            # An unsupported virtual interface must not hide physical ports.
            continue
        known = interface_cache.get(key, (0, []))[1]
        merged = list(entries) + [
            p for p in known if p["index"] not in {e["index"] for e in entries}
        ]
        interface_cache[key] = time.monotonic(), merged
        if not any(
            p["up"] and p["name"].startswith(("eth", "en")) for p in known
        ) and any(p["up"] and p["name"].startswith(("eth", "en")) for p in entries):
            interface_scan[key] = cursor, copy.deepcopy(entries)
            return merged
        if len(entries) == 16:
            break
    interface_scan.pop(key, None)
    if not failed:
        interface_cache[key] = time.monotonic(), entries
    return interface_cache[key][1]


def sample(cfg, group=None):
    if len(profiles) > 6:
        profiles.clear()
    result = dict(
        cpu=-1,
        memory=-1,
        storage=-1,
        temperature=-1,
        rx=-1,
        tx=-1,
        memoryTotal=0,
        memoryAvail=0,
        volumeTotal=0,
        volumeUsed=0,
        model="",
        volume="",
        interface="",
        uptime=0,
        online=False,
    )

    def sn(oid):
        try:
            return query(*target_key(cfg), oid)[1]
        except TimeoutError:
            # Retry one missing OID, rather than discarding the whole snapshot.
            time.sleep(0.5)
            return query(*target_key(cfg), oid)[1]

    key = target_key(cfg)
    profile = profiles.setdefault(key, Profile())
    def read(oid, next_oid=False):
        if oid == '1.3.6.1.4.1.6574.1.5.1.0' and not profile.synology_confirmed and state['page'] != 4:
            # Only this metadata worker waits; the FIFO slot stays free for
            # the first live snapshots. Match the firmware's ten-second bound.
            while not all(g in collection_stamps for g in ('cpu','memory')) and time.monotonic()-_startup_at < 10:
                if key != target_key(config): raise OSError('Collector target changed')
                time.sleep(.1)
        try:
            return query(*key, oid, next_oid)
        except TimeoutError:
            time.sleep(0.5)
            return query(*key, oid, next_oid)

    for kind in ("identity", "uptime", "temperature", "cpu", "memory"):
        if group is not None and group != kind:
            continue
        try:
            if kind == "identity":
                result["model"] = profile.identity(read)
            elif kind == "uptime":
                result["uptime"] = int(read("1.3.6.1.2.1.25.1.1.0")[1]) // 100
            elif kind == "temperature":
                value = profile.temperature(read)
                if value is not None: result["temperature"] = value
            elif kind == "cpu":
                value = profile.cpu(read)
                if value is not None: result["cpu"] = value
                result["_ok"] = profile.cpu_mode != "unsupported"
            elif kind == "memory":
                value = profile.memory(read)
                if value:
                    total,available = value
                    result.update(memoryTotal=total,memoryAvail=available,memory=100*(total-available)/total)
        except (OSError,ValueError,ArithmeticError,TypeError):
            pass
    if group is None or group == "storage":
        try:
            idx = cfg["storage"] or "54"
            units = sn("1.3.6.1.2.1.25.2.3.1.4." + idx)
            size, used = sn("1.3.6.1.2.1.25.2.3.1.5." + idx), sn(
                "1.3.6.1.2.1.25.2.3.1.6." + idx
            )
            result.update(
                volumeTotal=size * units,
                volumeUsed=used * units,
                storage=100 * used / size,
                volume=sn("1.3.6.1.2.1.25.2.3.1.3." + idx),
            )
        except (OSError, ValueError, ZeroDivisionError):
            pass
    if group is None or group == "network":
        try:
            interfaces = (
                interface_cache.get(target_key(cfg), (0, []))[1]
                if group == "network"
                else discover_interfaces(cfg)
            )
            result["interfaces"] = interfaces
            active = [p for p in interfaces if p["up"]]
            physical = [p for p in active if p["name"].startswith(("eth", "en"))]
            idx = cfg["iface"] or next((p["index"] for p in physical or active), "")
            if not idx:
                raise ValueError("No active Ethernet interface")
            result["interfaceIndex"] = idx
            values = profile.network(read, idx)
            if values: result.update(rx=values[0],tx=values[1])
            result["_ok"] = profile.network_mode != "unsupported"
            result["interface"] = next(
                (p["name"] for p in interfaces if p["index"] == idx), idx
            )
        except (OSError, ValueError, ZeroDivisionError):
            pass
    result["online"] = any(
        result[k] >= 0 for k in ("temperature", "memory", "cpu", "rx", "tx")
    )
    return result


def merge_sample(current, result, sampled_at, now):
    """Retain a recent good group during one timeout; expire it after the bounded cache lifetime."""
    groups = {
        "cpu": ("cpu",),
        "memory": ("memory", "memoryTotal", "memoryAvail"),
        "storage": ("storage", "volumeTotal", "volumeUsed", "volume"),
        "temperature": ("temperature",),
        "rx": ("rx", "tx", "interface", "interfaceIndex"),
    }
    grouped = {field for fields in groups.values() for field in fields}
    current.update(
        {
            k: v
            for k, v in result.items()
            if k not in grouped and (v != "" or k not in ("model",))
        }
    )
    for primary, fields in groups.items():
        if result.get(primary, -1) >= 0:
            sampled_at[primary] = now
            current.update({k: result[k] for k in fields if k in result})
        elif now - sampled_at.get(primary, -math.inf) > 30:
            current.update({k: result[k] for k in fields if k in result})


COLLECTION_FIELDS = {
    "cpu": ("cpu",),
    "memory": ("memory", "memoryTotal", "memoryAvail"),
    "network": ("rx", "tx", "interface", "interfaceIndex", "interfaces"),
    "temperature": ("temperature",),
    "uptime": ("uptime",),
    "identity": ("model",),
}
collection_stamps = {}
collection_diagnostics = {}
collection_target = None


def target_key(cfg):
    credentials = (
        Credentials(
            cfg.get("snmpUser", ""),
            cfg.get("snmpPassword", ""),
            cfg.get("snmpAuth", "MD5"),
            cfg.get("snmpPrivacy", "none"),
            cfg.get("snmpPrivacyPassword", ""),
        )
        if cfg.get("snmpVersion") == "3"
        else cfg["community"]
    )
    return cfg["nas"], cfg["port"], credentials


def storage_order(value):
    return tuple(
        (0, int(part)) if part.isdigit() else (1, part)
        for part in re.split(r"([0-9]+)", str(value))
        if part
    )


def reset_network_baseline():
    profile = profiles.get(target_key(config))
    if profile is not None: profile.network_previous = None


def expire_collections(now, cfg):
    ttl = max(30, cfg["interval"] * 6)
    cache_ttl = max(60, ttl)
    for group, primary in (
        ("cpu", "cpu"),
        ("memory", "memory"),
        ("network", "rx"),
        ("temperature", "temperature"),
    ):
        if now - collection_stamps.get(group, -math.inf) > cache_ttl:
            if group == "network" and state[primary] >= 0:
                reset_network_baseline()
            state[primary] = -1
            if group == "network":
                state["tx"] = -1
    state["online"] = now - last_ok <= ttl if last_ok else False


def collection_worker(group):
    global last_ok
    failures, due, seen, suspended = 0, 0, None, False
    while True:
        if not activity_allowed("snmp"):
            suspended = True
            time.sleep(0.2)
            continue
        if suspended:
            suspended = False
            if not failures: due = 0
            if group == "network": reset_network_baseline()
        with lock:
            cfg = copy.deepcopy(config)
        key = target_key(cfg)
        selection = key + (cfg["iface"],) if group == "network" else key
        if seen != selection:
            seen, due, failures = selection, 0, 0
            if group == "network":
                with lock:
                    reset_network_baseline()
                    collection_stamps.pop("network", None)
                    state["rx"] = state["tx"] = -1
        now = time.monotonic()
        if now < due or not startup_ready(cfg, group):
            time.sleep(0.1)
            continue
        sample_started = now
        try:
            result = sample(cfg, group)
            primary = COLLECTION_FIELDS[group][0]
            ok = result.get("_ok", False) or (
                bool(result.get(primary))
                if group == "identity"
                else result.get(primary, -1) >= 0
                and (group != "uptime" or result["uptime"] > 0)
            )
        except Exception as error:
            result, ok = {}, False
            print(f"SNMP {group}: {error}", file=sys.stderr, flush=True)
        now = time.monotonic()
        with lock:
            if key == collection_target and (
                group != "network" or cfg["iface"] == config["iface"]
            ):
                if ok:
                    if group not in ("cpu", "network") or result.get(primary, -1) >= 0:
                        collection_stamps[group] = now
                    last_ok = now
                    state.update(
                        {
                            field: result[field]
                            for field in COLLECTION_FIELDS[group]
                            if field in result
                            and (field not in ("cpu", "rx", "tx") or result[field] >= 0)
                        }
                    )
                    if group == "network" and result.get("rx", -1) >= 0:
                        state["history"] = (
                            state["history"] + [[result["rx"], result["tx"]]]
                        )[-30:]
                failures = 0 if ok else min(4, failures + 1)
                collection_diagnostics[group] = dict(
                    failures=failures, updated=collection_stamps.get(group)
                )
        period = (
            3600
            if group == "identity"
            else (
                7200
                if group == "uptime"
                else (
                    15
                    if group == "temperature"
                    else (
                        max(15, cfg["interval"])
                        if group in ("cpu", "memory")
                        else cfg["interval"]
                    )
                )
            )
        )
        prime = ok and group in ("cpu", "network") and result.get(primary, -1) < 0
        wait = 1 if prime else max(1, period - (now - sample_started))
        due = now + (wait if ok else min(120, max(15, period) * 2 ** (failures - 1)))


def interface_poll():
    seen, due, failures = None, 0, 0
    while True:
        if not activity_allowed("snmp"):
            time.sleep(0.2)
            continue
        with lock:
            cfg = copy.deepcopy(config)
        key = target_key(cfg)[:3]
        if key != seen:
            seen, due, failures = key, 0, 0
        if time.monotonic() < due or not startup_ready(cfg, "interfaces"):
            time.sleep(0.1)
            continue
        try:
            discover_interfaces(cfg)
            failures = 0
        except (OSError, ValueError, KeyError, TypeError):
            failures = min(4, failures + 1)
        due = time.monotonic() + (
            300 if not failures else min(120, 15 * 2 ** (failures - 1))
        )


def poll():
    global collection_target, last_ok, collecting, selection_save_at
    collecting = True
    # One worker owns each coherent snapshot. UI edits do not reset counter baselines.
    with lock:
        collection_target = target_key(config)
    threading.Thread(target=interface_poll, daemon=True).start()
    for group in COLLECTION_FIELDS:
        threading.Thread(target=collection_worker, args=(group,), daemon=True).start()
    while True:
        with lock:
            if selection_save_at and time.monotonic() >= selection_save_at:
                try:
                    persist_config()
                    selection_save_at = 0.0
                except OSError:
                    selection_save_at = time.monotonic() + 5.0
            key = target_key(config)
            if key != collection_target:
                collection_target = key
                profiles.clear()
                collection_stamps.clear()
                collection_diagnostics.clear()
                table_stamps.clear()
                last_ok = 0
                disks.clear()
                volumes.clear()
                state.update(
                    cpu=-1,
                    memory=-1,
                    rx=-1,
                    tx=-1,
                    temperature=-1,
                    model="",
                    uptime=0,
                    history=[],
                    storage=-1,
                )
            if activity_allowed("snmp"):
                expire_collections(time.monotonic(), config)
                expire_tables(time.monotonic())
            if weather["valid"] and time.monotonic() - weather_at > max(
                1800, config["weatherInterval"] * 180
            ):
                weather["valid"] = False
            state["mode"] = config["mode"]
            state["interfaces"] = copy.deepcopy(interface_cache.get(key, (0, []))[1])
            if volumes:
                v = next(
                    (v for v in volumes if v.get("index") == config["storage"]),
                    volumes[0],
                )
                state.update(
                    volume=v["name"],
                    volumeTotal=v["total"],
                    volumeUsed=v.get("used") or 0,
                    storage=(
                        100 * v["used"] / v["total"]
                        if v.get("valid", True) and v["total"]
                        else -1
                    ),
                )
        time.sleep(0.1)


def render(page, data, cfg):
    numbers = [
        page,
        data["cpu"],
        data["memory"],
        data["storage"],
        data["temperature"],
        data["rx"],
        data["tx"],
        data["memoryTotal"],
        data["memoryAvail"],
        data["volumeTotal"],
        data["volumeUsed"],
        data["uptime"],
        data["age"],
        int(data["online"]),
        data["rssi"],
    ]
    words = [
        cfg["nas"],
        data["ip"],
        (cfg.get("machineName") or profiles.get(target_key(cfg), Profile()).automatic_name()).replace(" ", "_"),
        data["volume"] or "--",
        data["interface"] or "--",
        "SNMPv3" if cfg.get("snmpVersion") == "3" else "SNMPv2c",
    ]
    payload = (
        " ".join(map(str, numbers))
        + " "
        + " ".join(w.replace(" ", "_") for w in words)
        + " "
        + str(len(data["history"]))
    )
    for pair in data["history"]:
        payload += " " + " ".join(map(str, pair))
    w = data["weather"]
    dt = datetime.now(timezone(timedelta(hours=8)))
    payload += (
        " "
        + " ".join(
            map(
                str,
                [
                    cfg["weatherEnabled"],
                    int(w["valid"]),
                    w["temperature"],
                    w["humidity"],
                    w["aqi"],
                    w["code"],
                ],
            )
        )
        + "\n"
    )
    payload += (
        "\n".join(
            str(v).replace("\n", " ").replace("\t", " ")
            for v in [
                (w["city"] if cfg["language"] == "zh" else w["cityEnglish"]) if cfg["cityCode"] in CITY_CODES else (w["cityEnglish"] or "Weather"),
                w["condition"] if cfg["language"] == "zh" else w["conditionEnglish"],
                w.get("wind", "") if cfg["language"] == "zh" else w["windEnglish"],
                w["observed"],
                dt.strftime("%H:%M:%S"),
                dt.strftime("%Y-%m-%d"),
            ]
        )
        + "\n"
    )
    a = data["alarm"]
    payload += (
        " ".join(
            map(
                str,
                [
                    cfg["alarmEnabled"],
                    a["state"],
                    a["remaining"],
                    cfg["alarmTime"],
                    len(data["disks"]),
                ],
            )
        )
        + "\n"
    )
    for disk in data["disks"]:
        payload += (
            " ".join(
                map(
                    str,
                    [
                        (disk["name"] or "Disk").replace(" ", "_"),
                        (disk["model"] or "--").replace(" ", "_"),
                        disk["temperature"],
                        disk["status"],
                        disk["health"],
                    ],
                )
            )
            + "\n"
        )
    payload += str(len(data["volumes"])) + "\n"
    for volume in data["volumes"]:
        payload += f"{volume['name']} 1 {volume['total']} {volume.get('used') or 0} {int(volume.get('valid', True))}\n"
    payload += f"{cfg['language']} {cfg['clockColor']} {data['diskGroup']} {data['volumeGroup']} {int(data.get('resetPending',False))} {cfg['minuteColor']} {cfg['showSeconds']} {(dt.weekday()+1)%7} {int(time.monotonic()*1000/37)%10} {cfg.get('nasBackground','#21455c')} {int(cfg.get('snmpVersion')=='3' and cfg.get('snmpPrivacy') in ('DES','AES'))}\n"
    security = cfg.get("snmpAuth", "MD5") if cfg.get("snmpVersion") == "3" else "-"
    if security != "-" and cfg.get("snmpPrivacy", "none") != "none":
        security += "+" + cfg["snmpPrivacy"]
    payload += security + "\n"
    notice = (
        data.get("machineNotice", "")
        if time.monotonic() < data.get("machineNoticeUntil", 0)
        else ""
    )
    payload += (
        notice
        + "\nSNMP "
        + str(cfg.get("activeMachine", 0) + 1)
        + "/"
        + str(len(cfg.get("machines", [])) or 1)
        + "\n"
    )
    payload += cfg.get("cardBackground", "#103040") + "\n"
    payload += str(data.get("cityScrollMs", int((time.monotonic()-w.get("scrollStarted",time.monotonic()))*1000))) + "\n"
    text = subprocess.run(
        [
            str(
                os.environ.get(
                    "MOS2_RENDERER",
                    str(
                        ROOT
                        / "simulator"
                        / ("renderer.exe" if sys.platform == "win32" else "renderer")
                    ),
                )
            )
        ],
        input=payload.encode("utf8"),
        env={**os.environ, "MOS2_FONT_DIRECTORY": str(ROOT / "data/fonts")},
        capture_output=True,
        check=True,
        timeout=3,
    ).stdout.decode("utf8")
    commands = []
    for row in text.splitlines():
        parts = row.split("\t")
        commands.append(
            [parts[0]] + [int(v) for v in parts[1:6]] + [parts[6]]
            if parts[0] == "text"
            else [parts[0]] + list(map(int, parts[1:]))
        )
    return commands


def qr_commands(mode, cfg):
    import qrcode

    color = 0x222B
    white = 65535
    labels = json.loads(
        (ROOT / "simulator/qr-locales.json").read_text(encoding="utf8")
    ).get(cfg["language"], {})
    text = lambda key: labels.get(key, key)
    code = qrcode.QRCode(
        version=5,
        error_correction=qrcode.constants.ERROR_CORRECT_L,
        box_size=3,
        border=4,
    )
    code.add_data(
        "WIFI:T:WPA;S:MOS2-NAS-DEMO;P:12345678;;"
        if mode == "setup"
        else "http://127.0.0.1:8765/manage"
    )
    code.make(fit=False)
    out = [
        ["rect", 0, 0, 240, 240, color],
        [
            "text",
            120,
            21,
            3,
            0x365E,
            4,
            text("WI-FI SETUP" if mode == "setup" else "MANAGEMENT"),
        ],
        ["rect", 52, 42, 135, 135, white],
    ]
    for y, row in enumerate(code.get_matrix()):
        for x, value in enumerate(row):
            if value:
                out.append(["rect", 52 + x * 3, 42 + y * 3, 3, 3, 0])
    lines = (
        [
            "MOS2-NAS-DEMO",
            (
                text("Key: {key}").format(key="12345678")
                if mode == "setup" or default_password(cfg)
                else text("Default password changed")
            ),
            text("Open 192.168.4.1"),
        ]
        if mode == "setup"
        else [
            "127.0.0.1",
            (
                text("Key: {key}").format(key="12345678")
                if default_password(cfg)
                else text("Default password changed")
            ),
            text("Touch to return"),
        ]
    )
    for y, font, label in zip([187, 208, 229], [2, 2, 2], lines):
        out.append(
            [
                "text",
                120,
                y,
                2,
                (
                    white
                    if y == 187
                    else (
                        0xFD88
                        if mode == "setup" and y == 208
                        else 0x365E if mode == "setup" or y == 208 else 0xAE3A
                    )
                ),
                4,
                label,
            ]
        )
    return out


class LocalHTTPServer(ThreadingHTTPServer):
    """Bound connection workers so slow local clients cannot create unlimited threads."""

    daemon_threads = True
    request_queue_size = 16

    def __init__(self, *args, **kwargs):
        self.slots = threading.BoundedSemaphore(16)
        super().__init__(*args, **kwargs)

    def process_request(self, request, address):
        if not self.slots.acquire(blocking=False):
            self.shutdown_request(request)
            return
        try:
            super().process_request(request, address)
        except Exception:
            self.slots.release()
            raise

    def process_request_thread(self, request, address):
        try:
            super().process_request_thread(request, address)
        finally:
            self.slots.release()


class Handler(BaseHTTPRequestHandler):
    # Bound slow clients before parsing headers/body. The service is loopback-only.
    def setup(self):
        super().setup()
        self.connection.settimeout(5)

    def log_message(self, *args):
        pass

    def send(self, body, ctype="application/json", code=200, headers=None):
        if isinstance(body, dict) or isinstance(body, list):
            body = json.dumps(body, ensure_ascii=False)
        if isinstance(body, str):
            body = body.encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Cache-Control", (headers or {}).get("Cache-Control", "no-store"))
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("X-Frame-Options", "DENY")
        self.send_header("Referrer-Policy", "same-origin")
        self.send_header("Content-Length", str(len(body)))
        for key, value in (headers or {}).items():
            if key != "Cache-Control": self.send_header(key, value)
        self.end_headers()
        self.wfile.write(body)

    def signed_in(self, pending=False):
        cookie = SimpleCookie()
        try:
            cookie.load(self.headers.get("Cookie", ""))
            session = cookie["mos2_session"].value
        except (KeyError, ValueError):
            return False
        with lock:
            seen = sessions.get(session, 0)
            if not seen or time.monotonic() - seen > 1800:
                sessions.pop(session, None)
                return False
            sessions[session] = time.monotonic()
        return pending or not default_password(config)

    def entry(self, setup=False, error=""):
        action = "/wifi/save" if setup else "/login"
        template = (ROOT / "web/access.html").read_text(encoding="utf8")
        for key, value in dict(
            mode=(
                "setup"
                if setup
                else (
                    "change"
                    if self.signed_in(pending=True) and default_password(config)
                    else "login"
                )
            ),
            wifiClass="" if setup else "hidden",
            ssidDisabled="" if setup else "disabled",
            entryTitle="连接 Wi-Fi" if setup else "管理登录",
            entrySubmit="保存并连接" if setup else "登录",
            action=action,
            token=token,
            ssid=config["ssid"],
            error=error,
        ).items():
            template = template.replace(
                "{{" + key + "}}", html.escape(value, quote=True)
            )
        return self.send(template, "text/html; charset=utf-8")

    def do_GET(self):
        if not local_request(self.headers, self.server.server_address[1]):
            return self.send(dict(error="origin"), code=403)
        global reset_pending, ui_mode, album_position, album_at, _management_demand
        path = urlparse(self.path)
        with lock:
            if reset_pending and time.monotonic() - reset_started >= 10:
                reset_pending = False
            cfg, data = copy.deepcopy(config), copy.deepcopy(state)
            if data.get("uptime") and "uptime" in collection_stamps:
                data["uptime"] += max(
                    0, int(time.monotonic() - collection_stamps["uptime"])
                )
            entries = album_entries()
            if entries:
                album_position %= len(entries)
                if (
                    not cfg["albumFixed"]
                    and time.monotonic() - album_at >= cfg["albumInterval"]
                ):
                    album_position = (album_position + 1) % len(entries)
                    album_at = time.monotonic()
            data.update(
                albumSlot=entries[album_position]["slot"] if entries else -1,
                albumFixed=cfg["albumFixed"],
                wifiConnected=bool(cfg["ssid"]),
                wifiSsid=cfg["ssid"],
            )
            data["age"] = int(time.monotonic() - last_ok) if last_ok else 0
            data["weather"] = copy.deepcopy(weather)
            data["weather"].update(
                enabled=bool(cfg["weatherEnabled"]),
                age=int(time.monotonic() - weather_at) if weather_at else 0,
            )
            data.update(
                alarm=copy.deepcopy(alarm),
                disks=sorted(
                    copy.deepcopy(disks),
                    key=lambda d: storage_order(d.get("index", d["name"])),
                ),
                volumes=sorted(
                    copy.deepcopy(volumes), key=lambda v: storage_order(v["name"])
                ),
                albumCount=len(album_entries()),
                albumInterval=cfg["albumInterval"],
            )
            data.update(
                language=cfg["language"],
                uiMode=ui_mode,
                resetPending=reset_pending,
                album=album_entries(),
            )
            data["rotation"] = cfg["rotation"]
            data["clock"] = datetime.now(timezone(timedelta(hours=8))).strftime(
                "%H:%M:%S"
            )
        if path.path == "/":
            return self.send(
                (ROOT / "simulator/index.html")
                .read_text(encoding="utf8")
                .replace("{{token}}", token),
                "text/html; charset=utf-8",
            )
        if path.path == "/api/wifi/scan":
            return self.send(
                dict(
                    scanning=False,
                    networks=[
                        dict(ssid="Demo Wi-Fi", rssi=-42, open=False),
                        dict(ssid="Guest 2.4G", rssi=-68, open=False),
                    ],
                )
            )
        if path.path == "/setup":
            if ui_mode == "setup" or not config["ssid"]:
                return self.entry(True)
            return self.send("", "text/plain", 303, {"Location": "/manage"})
        if path.path == "/manage":
            if not self.signed_in():
                return self.entry()
            h = (ROOT / "web/management.html").read_text(encoding="utf8")
            h = "\n".join(line.strip() for line in h.splitlines() if line.strip())
            for filename in ("i18n.js", "firmware-bundle.js", "management.js"):
                import hashlib
                digest = hashlib.sha256((ROOT / "web" / filename).read_text(encoding="utf8").encode("utf8")).hexdigest()[:16]
                h = re.sub(r"/" + re.escape(filename) + r"\?v=[^\"]+", "/" + filename + "?v=" + digest, h)
            h = re.sub(r'(<input name="ssid"[^>]*?)\srequired', r"\1", h)
            if not cfg["machines"]:
                cfg["machines"] = [{**{k: cfg[k] for k in MACHINE_FIELDS}, "name": cfg.get("machineName", "SNMP 1")} ]
                cfg["activeMachine"] = 0
            try:
                edit = int(parse_qs(path.query).get("device", [cfg["activeMachine"]])[0])
                if not 0 <= edit < len(cfg["machines"]): raise ValueError()
            except ValueError:
                return self.send("Invalid device", "text/plain", 400)
            selected = cfg["machines"][edit]
            cfg.update({k: selected[k] for k in MACHINE_FIELDS})
            cfg["machineName"] = selected["name"]
            cfg["selectedMachine"] = edit
            public = {
                k: v
                for k, v in cfg.items()
                if k
                not in (
                    "pass",
                    "admin",
                    "community",
                    "snmpPassword",
                    "snmpPrivacyPassword",
                )
            }
            public["machines"] = [
                {"name": m["name"], "address": m["nas"]} for m in cfg["machines"]
            ]
            h = (
                h.replace(
                    "{{config}}",
                    json.dumps(public, ensure_ascii=False).replace("<", "\\u003c"),
                )
                .replace("{{token}}", token)
                .replace("{{credentials}}", "")
                .replace("{{environment}}", "")
            )
            return self.send(h, "text/html; charset=utf-8")
        if path.path.startswith("/assets/"):
            asset = ROOT / "simulator/assets" / Path(path.path).name
            if asset.is_file() and asset.suffix == ".jpg":
                return self.send(asset.read_bytes(), "image/jpeg")
        if path.path in (
            "/weather-cities.js",
            "/i18n.js",
            "/management.js",
            "/firmware-bundle.js",
            "/setup.js",
            "/access.js",
            "/access-i18n.js",
        ):
            return self.send(
                (ROOT / "web" / Path(path.path).name).read_text(encoding="utf8"),
                "application/javascript; charset=utf-8",
                headers={"Cache-Control": "private, max-age=31536000, immutable"} if path.query.startswith("v=") else None,
            )
        if path.path == "/api/album":
            return self.send(album_entries())
        if path.path == "/api/album/image":
            try:
                slot = int(parse_qs(path.query)["slot"][0])
                p = album_dir / f"photo-{slot}.jpg"
                if not 0 <= slot < 7 or not p.exists():
                    raise ValueError()
                return self.send(p.read_bytes(), "image/jpeg")
            except (ValueError, KeyError):
                return self.send("No photo", "text/plain", 404)
        if path.path == "/api/status":
            data["displayName"] = cfg.get("machineName") or profiles.get(target_key(cfg), Profile()).automatic_name()
            if "management" in parse_qs(path.query) and not self.signed_in():
                return self.send(dict(error="login_required"), code=401)
            if "management" in parse_qs(path.query): _management_demand = time.monotonic()
            return self.send(data)
        if path.path == "/api/display":
            try:
                page = int(parse_qs(path.query).get("page", [data["page"]])[0])
                if page not in (
                    [0, 1, 2, 4, 6, 7, 8, 9] + ([5] if cfg["weatherEnabled"] else [])
                ):
                    raise ValueError("Invalid page")
                return self.send(
                    dict(
                        commands=(
                            qr_commands(ui_mode, cfg)
                            if ui_mode and not reset_pending
                            else render(page, data, cfg)
                        ),
                        status=data,
                    )
                )
            except Exception as e:
                return self.send(dict(error=str(e)), code=400)
        return self.send("Not found", "text/plain", 404)

    def do_POST(self):
        global revision, alarm_snooze_used, alarm_manual, alarm_start, alarm_taps, alarm_day, ui_mode, reset_pending, reset_started, reset_count, reset_tap_at, tap_count, tap_at, reset_nonce, reset_nonce_at, album_position, album_at
        if default_password(config) and urlparse(self.path).path not in (
            "/login",
            "/password/change",
            "/logout",
            "/wifi/save",
            "/api/touch",
            "/api/simulator/page",
        ):
            return self.send(dict(error="password_change_required"), code=403)
        if not local_request(self.headers, self.server.server_address[1]):
            self.close_connection = True
            return self.send(dict(error="origin"), code=403)
        path = urlparse(self.path).path
        # A public CSRF token is not an authentication credential.
        if (
            path not in ("/login", "/wifi/save", "/password/change", "/logout")
            and not self.signed_in()
        ):
            return self.send(dict(error="login_required"), code=401)
        if (
            self.headers.get("Transfer-Encoding")
            or len(self.headers.get_all("Content-Length", [])) != 1
        ):
            self.close_connection = True
            return self.send(dict(error="length"), code=400)
        try:
            text_length = self.headers["Content-Length"]
            if not re.fullmatch(r"[0-9]{1,10}", text_length):
                raise ValueError()
            length = int(text_length)
        except (ValueError, TypeError):
            self.close_connection = True
            return self.send(dict(error="length"), code=400)
        if urlparse(self.path).path == "/api/firmware":
            if not self.signed_in():
                return self.send(dict(error="login_required"), code=401)
            if self.headers.get("X-CSRF-Token") != token:
                return self.send(dict(error="csrf"), code=403)
            if length > 1048576 + 8192:
                return self.send(dict(error="size"), code=413)
            try:
                import zlib
                from email.parser import BytesParser
                from email.policy import default

                args = parse_qs(urlparse(self.path).query)
                size = int(args["size"][0])
                checksum = int(args["crc"][0], 16)
                if not 4096 <= size <= 1048576:
                    return self.send(dict(error="size"), code=400)
                message = BytesParser(policy=default).parsebytes(
                    (
                        "Content-Type: "
                        + self.headers["Content-Type"]
                        + "\r\nMIME-Version: 1.0\r\n\r\n"
                    ).encode()
                    + self.rfile.read(length)
                )
                files = [p for p in message.iter_parts() if p.get_filename()]
                if len(files) != 1:
                    raise ValueError("Expected one firmware")
                content = files[0].get_payload(decode=True)
                resource = (ROOT / "data/fonts/catalog.bin").read_bytes()
                contract = (
                    b"MOS2OTA1" + (0x200000).to_bytes(4, "little") + resource[12:16]
                )
                if (
                    len(content) != size
                    or content[0] != 0xE9
                    or contract not in content
                    or zlib.crc32(content) != checksum
                ):
                    raise ValueError("Invalid firmware")
                preserved = args.get("keep", ["1"])[0] != "0"
                if not preserved:
                    with lock:
                        erase_album()
                        config.update(copy.deepcopy(defaults))
                        clear_telemetry()
                        sessions.clear()
                        persist_config()
                        state["page"] = 6
                        ui_mode = "setup"
                        revision += 1
                return self.send(
                    dict(updated=True, preserved=preserved, simulated=True)
                )
            except (
                ValueError,
                KeyError,
                StopIteration,
                OSError,
                TypeError,
                AttributeError,
            ):
                return self.send(dict(error="invalid"), code=400)
        if urlparse(self.path).path == "/api/album/upload":
            if self.headers.get("X-CSRF-Token") != token:
                return self.send("Reload page", "text/plain", 403)
            if length > 55000:
                return self.send("Too large", "text/plain", 413)
            try:
                from email.parser import BytesParser
                from email.policy import default

                query_args = parse_qs(urlparse(self.path).query)
                if query_args.get("mode") == ["add"]:
                    occupied = {e["slot"] for e in album_entries()}
                    slot = next((i for i in range(7) if i not in occupied), -1)
                else:
                    slot = int(query_args["slot"][0])
                if not 0 <= slot < 7:
                    raise ValueError("Invalid slot")
                message = BytesParser(policy=default).parsebytes(
                    (
                        "Content-Type: "
                        + self.headers["Content-Type"]
                        + "\r\nMIME-Version: 1.0\r\n\r\n"
                    ).encode()
                    + self.rfile.read(length)
                )
                part = next(p for p in message.iter_parts() if p.get_filename())
                content = part.get_payload(decode=True)
                if len(content) > 49152:
                    raise ValueError("Too large")
                # Restrict the decoder itself, before format inspection, to JPEG.
                im = Image.open(io.BytesIO(content), formats=["JPEG"])
                if (
                    im.format != "JPEG"
                    or im.size != (240, 240)
                    or im.info.get("progressive")
                ):
                    raise ValueError("Expected baseline 240x240 JPEG")
                im.load()
                target = album_dir / f"photo-{slot}.jpg"
                temp = album_dir / "incoming.jpg"
                temp.write_bytes(content)
                temp.replace(target)
                return self.send({})
            except (ValueError, KeyError, StopIteration, OSError) as e:
                return self.send(str(e), "text/plain", 400)
        if length > 4096:
            return self.send("Too large", "text/plain", 413)
        try:
            body = self.rfile.read(length)
            if len(body) != length:
                raise ValueError()
            form = {
                k: v[0]
                for k, v in parse_qs(
                    body.decode(), keep_blank_values=True, max_num_fields=64
                ).items()
            }
        except (UnicodeDecodeError, ValueError):
            return self.send(dict(error="invalid"), code=400)
        if form.get("token") != token:
            return self.send("Reload the page", "text/plain", 403)
        if self.path == "/login":
            error = ""
            with lock:
                if login_guard.blocked():
                    error = "wait"
                elif not password_valid(form.get("password", ""), config["admin"]):
                    login_guard.failed()
                    error = "wrong"
                else:
                    login_guard.clear()
            if error:
                return self.entry(error=error)
            cookie = SimpleCookie()
            cookie.load(self.headers.get("Cookie", ""))
            existing = cookie.get("mos2_session")
            with lock:
                now = time.monotonic()
                session = existing.value if existing and existing.value in sessions and now - sessions[existing.value] <= 1800 else secrets.token_hex(24)
                sessions.clear()  # One active management browser session.
                sessions[session] = now
            return self.send(
                "",
                "text/plain",
                303,
                {
                    "Location": "/manage",
                    "Set-Cookie": f"mos2_session={session}; Path=/; HttpOnly; SameSite=Strict",
                },
            )
        if self.path == "/logout":
            if not self.signed_in(pending=True):
                return self.send(dict(error="login_required"), code=401)
            with lock:
                cookie = SimpleCookie(self.headers.get("Cookie", ""))
                session = cookie.get("mos2_session")
                if session:
                    sessions.pop(session.value, None)
            return self.send(
                dict(signedOut=True),
                headers={
                    "Set-Cookie": "mos2_session=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict"
                },
            )
        if self.path == "/password/change":
            if not self.signed_in(pending=True) or not default_password(config):
                return self.send(dict(error="expired"), code=403)
            password = form.get("newPassword", "")
            if (
                not 8 <= len(password.encode()) <= 64
                or password == "12345678"
                or password != form.get("confirmPassword")
            ):
                return self.send(dict(error="passwordInvalid"), code=400)
            with lock:
                previous_password = config["admin"]
                config["admin"] = password
                try:
                    persist_config()
                except OSError:
                    config["admin"] = previous_password
                    return self.send(dict(error="saveFailed"), code=500)
                sessions.clear()
            return self.send(
                dict(changed=True),
                headers={
                    "Set-Cookie": "mos2_session=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict"
                },
            )
        if self.path == "/wifi/save":
            if ui_mode != "setup" and not self.signed_in():
                return self.send(dict(error="login_required"), code=401)
            ssid, password = form.get("ssid", ""), form.get("password", "")
            if not 0 < len(ssid.encode()) <= 32 or not 8 <= len(password) <= 63:
                return self.send(dict(error="invalid"), code=400)
            with lock:
                old = copy.deepcopy(config)
                config.update(ssid=ssid, **{"pass": password})
                try:
                    persist_config()
                except OSError:
                    config.clear()
                    config.update(old)
                    return self.send(dict(error="saveFailed"), code=500)
            return self.send(
                {
                    "saved": True,
                    "message": "模拟器不连接实际 Wi-Fi / Simulator does not join Wi-Fi",
                }
            )
        if default_password(config) and self.path not in (
            "/api/touch",
            "/api/simulator/page",
        ):
            return self.send(dict(error="password_change_required"), code=403)
        if self.path == "/api/language":
            if form.get("language") not in ("zh", "en"):
                return self.send("Invalid language", "text/plain", 400)
            with lock:
                old_language = config["adminLanguage"]
                config["adminLanguage"] = form["language"]
                try:
                    persist_config()
                except OSError:
                    config["adminLanguage"] = old_language
                    return self.send(dict(error="saveFailed"), code=500)
            return self.send({})
        if self.path == "/api/album/delete":
            try:
                slot = int(form["slot"])
                if not 0 <= slot < 7:
                    raise ValueError()
                (album_dir / f"photo-{slot}.jpg").unlink(missing_ok=True)
                return self.send({})
            except (ValueError, KeyError):
                return self.send("Invalid slot", "text/plain", 400)
        if self.path in ("/api/alarm/test", "/api/alarm/tap"):
            with lock:
                if self.path.endswith("test"):
                    alarm_manual = True
                    alarm.update(state=1)
                    alarm_start = time.time()
                    alarm_snooze_used = False
                    alarm_taps = 0
                    state["page"] = 6
                elif alarm["state"] in (1, 2):
                    alarm_tap()
                else:
                    return self.send("No active alarm", "text/plain", 400)
            return self.send(alarm)
        if self.path == "/api/reset/request":
            reset_nonce = secrets.token_hex(16)
            reset_nonce_at = time.monotonic()
            return self.send(dict(nonce=reset_nonce))
        if self.path == "/api/reset/confirm":
            if (
                not reset_nonce
                or form.get("nonce") != reset_nonce
                or time.monotonic() - reset_nonce_at > 15
            ):
                return self.send("Confirmation expired", "text/plain", 403)
            reset_nonce = ""
            with lock:
                erase_album()
                config.update(copy.deepcopy(defaults))
                clear_telemetry()
                persist_config()
                sessions.clear()
                state["page"] = 6
                ui_mode = "setup"
                reset_pending = False
                revision += 1
            return self.send({})
        if self.path == "/api/storage/rediscover":
            global storage_discovery_generation, storage_discovery_requested_at
            with lock:
                now = time.monotonic()
                if (
                    storage_discovery_requested_at
                    and now - storage_discovery_requested_at < 60
                ):
                    return self.send("Discovery already requested", "text/plain", 429)
                storage_discovery_requested_at = now
                storage_discovery_generation += 1
            return self.send({})
        if self.path == "/api/machine":
            with lock:
                before = copy.deepcopy(config)
                try:
                    edited = machine_action(form)
                except ValueError:
                    return self.send("Invalid device action", "text/plain", 400)
                except OSError:
                    return self.send("Save failed", "text/plain", 500)
            public = {k:v for k,v in config.items() if k not in ("pass", "admin", "community", "snmpPassword", "snmpPrivacyPassword", "machines")}
            public.update({k:v for k,v in config["machines"][edited].items() if k not in ("community", "snmpPassword", "snmpPrivacyPassword", "name")})
            public["machineName"] = config["machines"][edited]["name"]
            public["machines"] = [{"name":m["name"], "address":m["nas"]} for m in config["machines"]]
            public["selectedMachine"] = edited
            return self.send(public)

        if self.path == "/api/touch":
            try:
                count = int(
                    form.get(
                        "count",
                        {
                            "tap": 1,
                            "double": 2,
                            "triple": 3,
                            "four": 4,
                            "five": 5,
                            "six": 6,
                            "seven": 7,
                            "eight": 8,
                        }.get(form.get("action"), 0),
                    )
                )
            except ValueError:
                return self.send("Invalid click count", "text/plain", 400)
            if count not in (1, 2, 3, 5, 6, 8, 9):
                return self.send("Invalid click count", "text/plain", 400)
            with lock:
                now = time.monotonic()
                if reset_pending:
                    if count == 3 and now - reset_started < 10:
                        erase_album()
                        config.update(copy.deepcopy(defaults))
                        clear_telemetry()
                        sessions.clear()
                        persist_config()
                        state["page"] = 6
                        ui_mode = "setup"
                        revision += 1
                    reset_pending = False
                elif alarm["state"] in (1, 2) and count in (1, 2, 3):
                    if count == 1:
                        alarm_tap()
                    elif count == 3:
                        alarm_taps = 0
                        for _ in range(3):
                            alarm_tap()
                elif count == 8:
                    reset_pending = True
                    reset_started = now
                elif count == 1:
                    if ui_mode:
                        ui_mode = ""
                    else:
                        next_page()
                elif count == 2:
                    if not ui_mode and state["page"] in (0, 4):
                        select_machine((config["activeMachine"] + 1) % len(config["machines"]), persist=False)
                    elif state["page"] == 8:
                        state["diskGroup"] = (state["diskGroup"] + 1) % max(
                            1, (len(disks) + 3) // 4
                        )
                    elif state["page"] == 9:
                        state["volumeGroup"] = (state["volumeGroup"] + 1) % max(
                            1, (len(volumes) + 2) // 3
                        )
                    elif state["page"] == 7 and album_entries():
                        album_position = (album_position + 1) % len(album_entries())
                        album_at = now
                elif count == 3:
                    state["page"] = 0 if state["page"] in (5, 6, 7) else 6
                    ui_mode = ""
                elif count == 5:
                    ui_mode = "" if ui_mode == "management" else "management"
                elif count == 6:
                    ui_mode = "setup"
            return self.send({})
        if self.path == "/api/beep":
            return self.send({})
        if self.path in ("/api/page", "/api/simulator/page"):
            try:
                page = int(form["page"])
                if page not in (
                    [0, 1, 2, 4, 6, 7, 8, 9] + ([5] if config["weatherEnabled"] else [])
                ):
                    raise ValueError()
                with lock:
                    state["page"] = page
                    ui_mode = ""
                    reset_pending = False
                    tap_count = 0
                return self.send({})
            except (ValueError, KeyError):
                return self.send("Invalid page", "text/plain", 400)
        if self.path == "/api/weather/check":
            code=form.get("code", "")
            if not re.fullmatch(r"[1-9][0-9]{8}",code): return self.send(dict(status="invalid"),code=400)
            with lock:
                if weather_code_ready(code): return self.send(dict(status="valid"))
                if weather_validation["status"]=="pending":
                    return self.send(dict(status="pending" if weather_validation["code"]==code else "busy"))
                if weather_validation["code"]==code and time.monotonic()-weather_validation["completed"]<3:
                    return self.send(dict(status=weather_validation["status"]))
                weather_validation.update(code=code,status="pending",completed=0)
                threading.Thread(target=probe_weather_code,args=(code,),daemon=True).start()
            return self.send(dict(status="pending"))
        if self.path == "/save":
            if re.fullmatch(r"10[12][0-9]{6}",form.get("cityCode", "")) and not weather_code_ready(form["cityCode"]):
                return self.send("Verify the city code before saving.", "text/plain", 409)
            try:
                with lock:
                    if not config["machines"]: capture_machine()
                    edit = int(form.get("machineIndex", config["activeMachine"]))
                    if not 0 <= edit < len(config["machines"]): raise ValueError()
                    new = copy.deepcopy(config)
                    selected = new["machines"][edit]
                    new.update({k: selected[k] for k in MACHINE_FIELDS})
                    new["machineName"] = selected["name"]
                ipaddress.IPv4Address(form["nas"])
                for k in (
                    "port",
                    "interval",
                    "brightness",
                    "home",
                    "weatherEnabled",
                    "weatherInterval",
                    "rotation",
                    "sound",
                    "alarmEnabled",
                    "snooze",
                    "albumInterval",
                    "alarmDays",
                    "alarmTune",
                    "albumFixed",
                    "showSeconds",
                ):
                    new[k] = int(form[k])
                if not (
                    1 <= new["port"] <= 65535
                    and 3 <= new["interval"] <= 60
                    and 5 <= new["brightness"] <= 100
                    and new["weatherEnabled"] in (0, 1)
                    and new["home"]
                    in (
                        [0, 1, 2, 4, 6, 7, 8, 9]
                        + ([5] if new["weatherEnabled"] else [])
                    )
                    and new["sound"] in (0, 1)
                    and new["alarmEnabled"] in (0, 1)
                    and 1 <= new["snooze"] <= 30
                    and 3 <= new["albumInterval"] <= 300
                    and 10 <= new["weatherInterval"] <= 120
                    and 0 <= new["rotation"] <= 3
                ):
                    raise ValueError()
                if not re.fullmatch(
                    "10[12][0-9]{6}", form["cityCode"]
                ) or not re.fullmatch("[a-zA-Z0-9.-]{1,63}", form["ntpServer"]):
                    raise ValueError()
                if not re.fullmatch(
                    r"(?:[01][0-9]|2[0-3]):[0-5][0-9]", form["alarmTime"]
                ):
                    raise ValueError()
                if (
                    new["showSeconds"] not in (0, 1)
                    or new["albumFixed"] not in (0, 1)
                    or not re.fullmatch("#[a-fA-F0-9]{6}", form["minuteColor"])
                    or form["language"] not in ("zh", "en")
                    or not re.fullmatch("#[a-fA-F0-9]{6}", form["clockColor"])
                    or not 1 <= new["alarmDays"] <= 127
                    or not 0 <= new["alarmTune"] <= 3
                ):
                    raise ValueError()
                if not re.fullmatch(
                    "#[a-fA-F0-9]{6}", form.get("nasBackground", "#21455c")
                ):
                    raise ValueError()
                new.update(
                    nasBackground=form.get("nasBackground", "#21455c"),
                    cardBackground=form.get("cardBackground", "#103040"),
                    language=form["language"],
                    clockColor=form["clockColor"],
                    minuteColor=form["minuteColor"],
                    cityCode=form["cityCode"],
                    ntpServer=form["ntpServer"],
                    alarmTime=form["alarmTime"],
                )
                for k in ("nas", "ssid", "storage", "iface"):
                    new[k] = form.get(k, "")
                for k in (
                    "community",
                    "pass",
                    "admin",
                    "snmpPassword",
                    "snmpPrivacyPassword",
                ):
                    if form.get(k) and (k != "admin" or not password_valid(form[k], config["admin"])):
                        new[k] = form[k]
                if form.get("admin") and (
                    len(form["admin"].encode()) < 8 or form["admin"] == "12345678"
                ):
                    raise ValueError()
                new["snmpVersion"] = form.get(
                    "snmpVersion", new.get("snmpVersion", "2c")
                )
                new["snmpUser"] = form.get("snmpUser", new.get("snmpUser", ""))
                new["snmpPrivacy"] = form.get(
                    "snmpPrivacy", new.get("snmpPrivacy", "none")
                )
                if (
                    new["snmpPrivacy"] not in ("none", "DES", "AES")
                    or len(new["snmpPrivacyPassword"].encode()) > 64
                ):
                    raise ValueError()
                if (
                    new["snmpVersion"] == "3"
                    and new["snmpPrivacy"] != "none"
                    and len(new["snmpPrivacyPassword"].encode()) < 8
                ):
                    raise ValueError()
                new["snmpAuth"] = form.get("snmpAuth", new.get("snmpAuth", "MD5"))
                if new["snmpVersion"] not in ("2c", "3") or new["snmpAuth"] not in (
                    "MD5",
                    "SHA",
                ):
                    raise ValueError()
                if (
                    len(new["snmpUser"].encode()) > 32
                    or len(new["snmpPassword"].encode()) > 64
                ):
                    raise ValueError()
                if new["snmpVersion"] == "3" and (
                    not new["snmpUser"] or len(new["snmpPassword"].encode()) < 8
                ):
                    raise ValueError()
                with lock:
                    if config["alarmTime"] != new["alarmTime"]:
                        alarm_day = ""
                    previous_config = copy.deepcopy(config)
                    new["machineName"] = form.get(
                        "machineName", new["machineName"]
                    ).strip()
                    if len(new["machineName"].encode()) > 48:
                        raise ValueError()
                    new["machines"][edit] = {**{k: new[k] for k in MACHINE_FIELDS}, "name": new["machineName"]}
                    current = new["machines"][config["activeMachine"]]
                    new["activeMachine"] = config["activeMachine"]
                    new.update({k: current[k] for k in MACHINE_FIELDS})
                    new["machineName"] = current["name"]
                    config.update(new)
                    try:
                        persist_config()
                    except OSError:
                        config.clear()
                        config.update(previous_config)
                        return self.send("Save failed", "text/plain", 500)
                    revision += 1
                    # Editing a profile does not change the visible page.
                password_changed = bool(form.get("admin")) and not password_valid(form["admin"], previous_config["admin"])
                if password_changed:
                    with lock: sessions.clear()
                return self.send("Saved. Restarting device.", "text/plain; charset=utf-8",
                                 headers={"X-MOS2-Save-Action": "restart"})
            except (ValueError, KeyError):
                return self.send("Invalid settings", "text/plain", 400)
        return self.send("Not found", "text/plain", 404)


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument(
        "--port", type=int, default=0 if getattr(sys, "frozen", False) else 8765
    )
    p.add_argument("--no-browser", action="store_true")
    p.add_argument("--config", type=Path, help="Private writable configuration file")
    args = p.parse_args()
    load_config(args.config or album_dir / "config.json")
    server = LocalHTTPServer(("127.0.0.1", args.port), Handler)
    address = f"http://127.0.0.1:{server.server_address[1]}/"
    if getattr(sys, "frozen", False) and not args.no_browser:
        import webbrowser

        threading.Timer(0.5, lambda: webbrowser.open(address)).start()
    threading.Thread(target=extra_poll, daemon=True).start()
    threading.Thread(target=alarm_poll, daemon=True).start()
    threading.Thread(target=poll, daemon=True).start()
    threading.Thread(target=weather_poll, daemon=True).start()
    print(f"MOS2 simulator {address}", flush=True)
    server.serve_forever()
