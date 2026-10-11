"""Local MOS2 screen simulator, using the firmware's C++ dashboard renderer."""

from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from pathlib import Path
import argparse, copy, json, math, subprocess, sys, threading, time, re, os
from datetime import datetime, timezone, timedelta
from urllib.parse import parse_qs, urlencode, urlparse
from urllib.request import urlopen
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
from snmp_probe import query as snmp_query
from snmp_v3 import Credentials, query as snmp_v3_query

# FIFO send budget prevents a long table walk from repeatedly reacquiring a
# semaphore ahead of another collector. Only reply waiting occupies a slot.
_snmp_condition = threading.Condition()
_snmp_sent_at = 0.0
_snmp_active = 0
_snmp_ticket = 0
_snmp_serving = 0
_snmp_failures = 0
_snmp_cooldown = 0.0


def query(host, port, community, name, next_oid=False):
    global _snmp_sent_at, _snmp_active, _snmp_ticket, _snmp_serving, _snmp_failures, _snmp_cooldown
    with _snmp_condition:
        ticket = _snmp_ticket
        _snmp_ticket += 1
        while True:
            wait = max(0.5 - (time.monotonic() - _snmp_sent_at), _snmp_cooldown - time.monotonic())
            if ticket == _snmp_serving and _snmp_active < 1 and wait <= 0:
                _snmp_active += 1
                _snmp_serving += 1
                _snmp_sent_at = time.monotonic()
                _snmp_condition.notify_all()
                break
            _snmp_condition.wait(max(0.001, min(0.075, wait)) if wait > 0 else 0.075)
    try:
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
            _snmp_failures += 1
            if _snmp_failures >= 3:
                _snmp_cooldown = time.monotonic() + 60
        raise
    finally:
        with _snmp_condition:
            _snmp_active -= 1
            _snmp_condition.notify_all()


config = dict(
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
    language="zh",
    adminLanguage="zh",
    clockColor="#ffffff",
    minuteColor="#ffa500",
    albumFixed=0,
    showSeconds=1,
    alarmDays=31,
    alarmTune=0,
    sound=0,
    alarmEnabled=0,
    alarmTime="07:00",
    snooze=5,
    albumInterval=10,
    weatherEnabled=1,
    cityCode="101030100",
    weatherInterval=10,
    rotation=0,
    ntpServer="ntp.aliyun.com",
    storage="",
    iface="",
    interval=5,
    brightness=70,
    home=6,
    ssid="",
    **{"pass": "", "admin": "12345678"},
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
revision = 0
previous = None
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


def persist_config():
    if config_path is None:
        return
    temporary = config_path.with_suffix(".tmp")
    temporary.write_text(
        json.dumps(config, ensure_ascii=False, indent=2), encoding="utf8"
    )
    temporary.replace(config_path)


def load_config(path):
    global config_path
    config_path = Path(path)
    if config_path.is_file():
        saved = json.loads(config_path.read_text(encoding="utf8"))
        config.update({key: saved[key] for key in defaults if key in saved})
    state["page"] = config["home"]


alarm = dict(enabled=0, time="07:00", state=0, remaining=0)
alarm_due = 0
alarm_start = 0
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
    disks.clear()
    volumes.clear()
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
    persist_config()
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
    global alarm_due, alarm_taps, alarm_tap_at, alarm_manual
    now = time.monotonic()
    alarm_taps = alarm_taps + 1 if now - alarm_tap_at <= 0.8 else 1
    alarm_tap_at = now
    if alarm_taps >= 3:
        alarm.update(state=3, remaining=0)
        alarm_manual = False
        alarm_taps = 0
    elif alarm["state"] == 1:
        alarm["state"] = 2
        alarm_due = time.time() + config["snooze"] * 60
        alarm["remaining"] = config["snooze"] * 60


def alarm_poll():
    global alarm_due, alarm_day, alarm_start, alarm_taps
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
                state["page"] = 6
                alarm_taps = 0
            if alarm["state"] == 2 and now >= alarm_due:
                alarm["state"] = 1
                alarm_start = now
                alarm_taps = 0
            if alarm["state"] == 1 and now - alarm_start >= 60:
                alarm["state"] = 2
                alarm_due = now + config["snooze"] * 60
            alarm["remaining"] = (
                max(0, int(alarm_due - now)) if alarm["state"] == 2 else 0
            )
        time.sleep(0.1)


table_stamps = {}


def table_poll(kind):
    global disks, volumes
    seen, metadata_at, due = None, 0, 0
    scan_cursor, scan_found = None, []
    while True:
        with lock:
            cfg = copy.deepcopy(config)
            rows = copy.deepcopy(disks if kind == "disk" else volumes)
        key = target_key(cfg)
        now = time.monotonic()
        if seen != key:
            seen, metadata_at, due, rows = key, 0, 0, []
            scan_cursor, scan_found = None, []
        if now < due or not startup_ready(cfg, kind):
            time.sleep(0.1)
            continue

        if kind == "disk" and not rows and state["page"] != 8 and not any(
            v.get("valid") for v in volumes
        ) and now - _startup_at < 120:
            time.sleep(0.1)
            continue

        def sn(oid, next_oid=False):
            return query(*target_key(cfg), oid, next_oid)

        ok = True
        if scan_cursor is not None or not rows or not metadata_at or now - metadata_at >= 7200:
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
                    if kind == "volume" and not re.fullmatch(
                        r"/volume[0-9]+", str(name)
                    ):
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
                        if units <= 0 or size <= 0:
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
                        if kind == "disk": disks = copy.deepcopy(rows)
                        else: volumes = copy.deepcopy(rows)
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
                if kind == "volume" and table_stamps.get((kind, row["index"], field), -math.inf) >= now:
                    continue  # Already acquired while discovering this row.
                if kind == "disk" and field != "temperature" and now - table_stamps.get((kind, row["index"], field), -math.inf) < 3600:
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
            (300 if kind == "disk" else 7200) if ok else (15 if not metadata_at else 120)
        )


def expire_tables(now):
    for kind, rows, ttl in (("disk", disks, 7200), ("volume", volumes, 14400)):
        for row in rows:
            for field in (
                ("status", "temperature", "health") if kind == "disk" else ("used",)
            ):
                if (
                    now - table_stamps.get((kind, row.get("index"), field), -math.inf)
                    > (900 if kind == "disk" and field == "temperature" else ttl)
                ):
                    row[field] = -1 if kind == "disk" else None
                    if kind == "volume":
                        row["valid"] = False


def extra_poll():
    for kind in ("disk", "volume"):
        threading.Thread(target=table_poll, args=(kind,), daemon=True).start()


def weather_poll():
    global weather_at
    seen = -1
    attempt = 0
    while True:
        with lock:
            cfg = copy.deepcopy(config)
            rev = (cfg["cityCode"], cfg["weatherEnabled"])
            error = weather["error"]
            if seen != -1 and seen != rev:
                weather.update(valid=False, city="", condition="")
                weather_at = 0
        now = time.monotonic()
        if cfg["weatherEnabled"] and (
            rev != seen
            or now - attempt >= (60 if error else cfg["weatherInterval"] * 60)
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
                h = float(data["SD"].replace("%", ""))
                if data["city"] != cfg["cityCode"] or not (
                    -80 <= t <= 65 and 0 <= h <= 100
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
                    conditionEnglish=data["weathere"],
                    windEnglish=data["wde"] + " " + data["wse"],
                    wind=data["WD"] + " " + data["WS"],
                    code=int(data["weathercode"][1:]),
                )
                with lock:
                    if rev == (config["cityCode"], config["weatherEnabled"]):
                        weather.update(update)
                        weather_at = time.monotonic()
            except (OSError, ValueError, KeyError, TypeError) as e:
                with lock:
                    if rev == (config["cityCode"], config["weatherEnabled"]):
                        weather["error"] = str(e)
        time.sleep(0.25)


cpu_previous = None


interface_cache = {}
interface_scan = {}


STARTUP_GROUPS = ("cpu", "memory", "network", "temperature", "uptime", "identity", "disk", "volume", "interfaces")
_startup_lock = threading.Lock()
_startup_key, _startup_page, _startup_due = None, None, {}
_storage_released, _startup_at = False, 0.0


def startup_delays(page):
    if page in (5, 6, 7):
        return dict(zip(STARTUP_GROUPS, (20, 21, 23, 22, 25, 30, 35, 40, 22)))
    delay = dict(zip(STARTUP_GROUPS, (30, 31, 32, 33, 45, 60, 65, 70, 30)))
    primary = {0: ("cpu", "memory", "network", "temperature", "interfaces"),
               1: ("network", "interfaces"), 2: ("cpu", "memory"),
               4: ("uptime", "identity", "interfaces"), 8: ("disk",), 9: ("volume",)}
    for group in primary.get(page, ()):
        delay[group] = 0
    return delay


def startup_ready(cfg, group):
    global _startup_key, _startup_page, _startup_due, _storage_released, _startup_at
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
                g in collection_stamps for g in ("cpu", "memory", "network", "temperature")
            )
            _storage_released |= ready or page in (8, 9) or now - _startup_at >= 120
            return _storage_released
        return now >= _startup_due[group]


def discover_interfaces(cfg):
    key = target_key(cfg)
    cached = interface_cache.get(key)
    if cached and time.monotonic() - cached[0] < 300 and any(p["up"] for p in cached[1]):
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
            if query(*key, "1.3.6.1.2.1.2.2.1.3." + index)[1] == 6:
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
        if not any(p["up"] and p["name"].startswith(("eth", "en")) for p in known) and any(
            p["up"] and p["name"].startswith(("eth", "en")) for p in entries
        ):
            interface_scan[key] = cursor, copy.deepcopy(entries)
            return merged
        if len(entries) == 16:
            break
    interface_scan.pop(key, None)
    if not failed:
        interface_cache[key] = time.monotonic(), entries
    return interface_cache[key][1]


def sample(cfg, group=None):
    global previous, cpu_previous
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

    if group is None or group == "temperature":
        try:
            result["temperature"] = sn("1.3.6.1.4.1.6574.1.2.0")
        except (OSError, ValueError):
            pass
    if group is None or group in ("uptime", "identity"):
        for field, oid in (
            ("model", "1.3.6.1.4.1.6574.1.5.1.0"),
            ("uptime", "1.3.6.1.2.1.25.1.1.0"),
        ):
            if group is not None and field != (
                "model" if group == "identity" else "uptime"
            ):
                continue
            try:
                value = sn(oid)
                result[field] = value // 100 if field == "uptime" else value
            except (OSError, ValueError):
                pass
    if group is None or group == "cpu":
        try:
            raw = [
                sn("1.3.6.1.4.1.2021.11." + str(n) + ".0")
                for n in (50, 51, 52, 53, 54, 56, 61, 64)
            ]
            key = target_key(cfg)
            if cpu_previous and cpu_previous[0] == key:
                delta = [(a - b) & 0xFFFFFFFF for a, b in zip(raw, cpu_previous[1])]
                if sum(delta) and all(v <= 0x7FFFFFFF for v in delta):
                    result["cpu"] = (
                        100 * (sum(delta) - delta[3] - delta[4]) / sum(delta)
                    )
            cpu_previous = (key, raw)
            result["_ok"] = True
        except (OSError, ValueError, ZeroDivisionError):
            pass
    if group is None or group == "memory":
        try:
            total, avail = sn("1.3.6.1.4.1.2021.4.5.0"), sn("1.3.6.1.4.1.2021.4.6.0")
            buffers, cache = sn("1.3.6.1.4.1.2021.4.14.0"), sn(
                "1.3.6.1.4.1.2021.4.15.0"
            )
            if total <= 0 or avail < 0 or avail > total or buffers < 0 or cache < 0:
                raise ValueError("Invalid memory snapshot")
            available = min(total, avail + buffers + cache)
            result.update(
                memoryTotal=total * 1024,
                memoryAvail=available * 1024,
                memory=100 * (total - available) / total,
                online=True,
            )
        except (OSError, ValueError, ZeroDivisionError):
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
            rx, tx = sn("1.3.6.1.2.1.31.1.1.1.6." + idx), sn(
                "1.3.6.1.2.1.31.1.1.1.10." + idx
            )
            now = time.monotonic()
            counter_key = target_key(cfg) + (idx,)
            if (
                previous
                and previous[3] == counter_key
                and rx >= previous[0]
                and tx >= previous[1]
            ):
                result.update(
                    rx=(rx - previous[0]) / (now - previous[2]),
                    tx=(tx - previous[1]) / (now - previous[2]),
                )
            previous = rx, tx, now, counter_key
            result["_ok"] = True
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


def expire_collections(now, cfg):
    global previous
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
                previous = None
            state[primary] = -1
            if group == "network":
                state["tx"] = -1
    state["online"] = now - last_ok <= ttl if last_ok else False


def collection_worker(group):
    global last_ok, previous
    failures, due, seen = 0, 0, None
    while True:
        with lock:
            cfg = copy.deepcopy(config)
        key = target_key(cfg)
        selection = key + (cfg["iface"],) if group == "network" else key
        if seen != selection:
            seen, due, failures = selection, 0, 0
            if group == "network":
                with lock:
                    previous = None
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
            3600 if group == "identity" else 7200 if group == "uptime" else 15 if group == "temperature" else max(15, cfg["interval"]) if group in ("cpu", "memory") else cfg["interval"]
        )
        prime = ok and group in ("cpu", "network") and result.get(primary, -1) < 0
        wait = 1 if prime else max(1, period - (now - sample_started))
        due = now + (wait if ok else min(120, max(15, period) * 2 ** (failures - 1)))


def interface_poll():
    seen, due, failures = None, 0, 0
    while True:
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
    global collection_target, previous, cpu_previous, last_ok
    # One worker owns each coherent snapshot. UI edits do not reset counter baselines.
    with lock:
        collection_target = target_key(config)
    threading.Thread(target=interface_poll, daemon=True).start()
    for group in COLLECTION_FIELDS:
        threading.Thread(target=collection_worker, args=(group,), daemon=True).start()
    while True:
        with lock:
            key = target_key(config)
            if key != collection_target:
                collection_target = key
                previous = cpu_previous = None
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
        data["model"] or "Synology",
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
                w["city"] if cfg["language"] == "zh" else w["cityEnglish"],
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
        env={**os.environ, "MOS2_FONT_RESOURCE": str(ROOT / "data/fonts/ui04.bin")},
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
                if mode == "setup" or cfg["admin"] == "12345678"
                else text("Default password changed")
            ),
            text("Open 192.168.4.1"),
        ]
        if mode == "setup"
        else [
            "127.0.0.1",
            (
                text("Key: {key}").format(key="12345678")
                if cfg["admin"] == "12345678"
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


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def send(self, body, ctype="application/json", code=200, headers=None):
        if isinstance(body, dict) or isinstance(body, list):
            body = json.dumps(body, ensure_ascii=False)
        if isinstance(body, str):
            body = body.encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        for key, value in (headers or {}).items():
            self.send_header(key, value)
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
        return pending or config["admin"] != "12345678"

    def entry(self, setup=False, error=""):
        action = "/wifi/save" if setup else "/login"
        template = (ROOT / "web/access.html").read_text(encoding="utf8")
        for key, value in dict(
            mode=(
                "setup"
                if setup
                else (
                    "change"
                    if self.signed_in(pending=True) and config["admin"] == "12345678"
                    else "login"
                )
            ),
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
        global reset_pending, ui_mode, album_position, album_at
        path = urlparse(self.path)
        with lock:
            if reset_pending and time.monotonic() - reset_started >= 10:
                reset_pending = False
            cfg, data = copy.deepcopy(config), copy.deepcopy(state)
            if data.get("uptime") and "uptime" in collection_stamps:
                data["uptime"] += max(0, int(time.monotonic() - collection_stamps["uptime"]))
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
                disks=copy.deepcopy(disks),
                volumes=copy.deepcopy(volumes),
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
            return self.entry(True)
        if path.path == "/manage":
            if not self.signed_in():
                return self.entry()
            h = (ROOT / "web/management.html").read_text(encoding="utf8")
            h = re.sub(r'(<input name="ssid"[^>]*?)\srequired', r"\1", h)
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
        global revision, previous, alarm_manual, alarm_start, alarm_taps, alarm_day, ui_mode, reset_pending, reset_started, reset_count, reset_tap_at, tap_count, tap_at, reset_nonce, reset_nonce_at, album_position, album_at
        if config["admin"] == "12345678" and urlparse(self.path).path not in (
            "/login",
            "/password/change",
            "/logout",
            "/wifi/save",
            "/api/touch",
            "/api/simulator/page",
        ):
            return self.send(dict(error="password_change_required"), code=403)
        length = int(self.headers.get("Content-Length", "0"))
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
                resource = (ROOT / "data/fonts/ui04.bin").read_bytes()
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
                im = Image.open(io.BytesIO(content))
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
        form = {
            k: v[0]
            for k, v in parse_qs(
                self.rfile.read(length).decode(), keep_blank_values=True
            ).items()
        }
        if form.get("token") != token:
            return self.send("Reload the page", "text/plain", 403)
        if self.path == "/login":
            if form.get("password") != config["admin"]:
                return self.entry(error="wrong")
            session = secrets.token_hex(24)
            with lock:
                sessions.clear()
                sessions[session] = time.monotonic()
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
            if not self.signed_in(pending=True) or config["admin"] != "12345678":
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
            ssid, password = form.get("ssid", ""), form.get("password", "")
            if not 0 < len(ssid.encode()) <= 32 or not 8 <= len(password) <= 63:
                return self.send(dict(error="invalid"), code=400)
            with lock:
                config.update(ssid=ssid, **{"pass": password})
                persist_config()
            return self.send(
                {
                    "saved": True,
                    "message": "模拟器不连接实际 Wi-Fi / Simulator does not join Wi-Fi",
                }
            )
        if config["admin"] == "12345678" and self.path not in (
            "/api/touch",
            "/api/simulator/page",
        ):
            return self.send(dict(error="password_change_required"), code=403)
        if self.path == "/api/language":
            if form.get("language") not in ("zh", "en"):
                return self.send("Invalid language", "text/plain", 400)
            with lock:
                config["adminLanguage"] = form["language"]
                persist_config()
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
                state["page"] = 6
                ui_mode = "setup"
                reset_pending = False
                revision += 1
            return self.send({})
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
            if count not in (1, 2, 3, 5, 6, 8):
                return self.send("Invalid click count", "text/plain", 400)
            with lock:
                now = time.monotonic()
                if reset_pending:
                    if count == 3 and now - reset_started < 10:
                        erase_album()
                        config.update(copy.deepcopy(defaults))
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
                    if state["page"] == 8:
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
        if self.path == "/save":
            try:
                with lock:
                    new = copy.deepcopy(config)
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
                    and 5 <= new["weatherInterval"] <= 60
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
                    if form.get(k):
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
                    config.update(new)
                    try:
                        persist_config()
                    except OSError:
                        config.clear()
                        config.update(previous_config)
                        return self.send("Save failed", "text/plain", 500)
                    revision += 1
                    state.update(page=new["home"])
                return self.send(
                    "Saved." if new["adminLanguage"] == "en" else "已保存。",
                    "text/plain; charset=utf-8",
                )
            except (ValueError, KeyError):
                return self.send("Invalid settings", "text/plain", 400)
        return self.send("Not found", "text/plain", 404)


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument(
        "--port", type=int, default=0 if getattr(sys, "frozen", False) else 8765
    )
    p.add_argument("--no-browser", action="store_true")
    args = p.parse_args()
    load_config(album_dir / "config.json")
    server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
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
