"""Local MOS2 screen simulator, using the firmware's C++ dashboard renderer."""

from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from pathlib import Path
import argparse, copy, json, math, subprocess, sys, threading, time, re
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
from snmp_probe import query

config = dict(
    nas="192.168.1.100",
    community="public",
    port=161,
    mode="snmp",
    nasBackground="#21455c",
    language="zh",
    adminLanguage="zh",
    clockColor="#ffffff",
    minuteColor="#ffa500",
    albumFixed=0,
    showSeconds=1,
    alarmDays=127,
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


def extra_poll():
    global disks, volumes
    while True:
        with lock:
            cfg = copy.deepcopy(config)

        def sn(oid, next=False):
            return query(cfg["nas"], cfg["port"], cfg["community"], oid, next)

        try:
            table = {}
            root = "1.3.6.1.4.1.6574.2.1.1"
            oid = root
            for _ in range(180):
                oid, value = sn(oid, True)
                if not oid.startswith(root + "."):
                    break
                column, index = oid[len(root) + 1 :].split(".")
                table.setdefault(index, {})[int(column)] = value
            newdisks = [
                dict(
                    name=v.get(2, "Disk"),
                    model=v.get(3, ""),
                    temperature=v.get(6, -1),
                    status=v.get(5, -1),
                    health=v.get(13, -1),
                )
                for v in table.values()
            ]
            root = "1.3.6.1.2.1.25.2.3.1.3"
            oid = root
            newvolumes = []
            for _ in range(128):
                oid, value = sn(oid, True)
                if not oid.startswith(root + "."):
                    break
                if isinstance(value, str) and re.fullmatch(r"/volume[0-9]+", value):
                    index = oid.rsplit(".", 1)[1]
                    units = sn("1.3.6.1.2.1.25.2.3.1.4." + index)[1]
                    size = sn("1.3.6.1.2.1.25.2.3.1.5." + index)[1]
                    used = sn("1.3.6.1.2.1.25.2.3.1.6." + index)[1]
                    newvolumes.append(
                        dict(name=value, total=units * size, used=units * used)
                    )
            with lock:
                disks = newdisks
                volumes = newvolumes
        except (OSError, ValueError, KeyError):
            pass
        time.sleep(30)


def weather_poll():
    global weather_at
    seen = -1
    attempt = 0
    while True:
        with lock:
            cfg, rev = copy.deepcopy(config), revision
            error = weather["error"]
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
                    if rev == revision:
                        weather.update(update)
                        weather_at = time.monotonic()
            except (OSError, ValueError, KeyError, TypeError) as e:
                with lock:
                    if rev == revision:
                        weather["error"] = str(e)
        time.sleep(0.25)


cpu_previous = None


interface_cache = {}

def discover_interfaces(cfg):
    key = (cfg["nas"], cfg["port"], cfg["community"])
    cached = interface_cache.get(key)
    if cached and time.monotonic() - cached[0] < 60:
        return cached[1]
    root = "1.3.6.1.2.1.2.2.1.2"
    cursor = root
    entries = []
    for _ in range(64):
        oid, name = query(*key, cursor, next=True)
        if not oid.startswith(root + "."):
            break
        cursor = oid
        index = oid.rsplit(".", 1)[1]
        if query(*key, "1.3.6.1.2.1.2.2.1.3." + index)[1] == 6:
            entries.append(dict(index=index, name=name,
                up=query(*key, "1.3.6.1.2.1.2.2.1.8." + index)[1] == 1))
        if len(entries) == 16:
            break
    interface_cache[key] = time.monotonic(), entries
    return entries


def sample(cfg):
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
        return query(cfg["nas"], cfg["port"], cfg["community"], oid)[1]

    try:
        result["temperature"] = sn("1.3.6.1.4.1.6574.1.2.0")
        result["model"] = sn("1.3.6.1.4.1.6574.1.5.1.0")
        result["uptime"] = sn("1.3.6.1.2.1.1.3.0") // 100
    except (OSError, ValueError):
        pass
    try:
        raw = [
            sn("1.3.6.1.4.1.2021.11." + str(n) + ".0")
            for n in (50, 51, 52, 53, 54, 56, 61, 64)
        ]
        key = (cfg["nas"], cfg["port"], cfg["community"])
        if cpu_previous and cpu_previous[0] == key:
            delta = [(a - b) & 0xFFFFFFFF for a, b in zip(raw, cpu_previous[1])]
            if sum(delta) and all(v <= 0x7FFFFFFF for v in delta):
                result["cpu"] = 100 * (sum(delta) - delta[3] - delta[4]) / sum(delta)
        cpu_previous = (key, raw)
        total, avail = sn("1.3.6.1.4.1.2021.4.5.0"), sn("1.3.6.1.4.1.2021.4.6.0")
        buffers, cache = sn("1.3.6.1.4.1.2021.4.14.0"), sn("1.3.6.1.4.1.2021.4.15.0")
        available = min(total, avail + buffers + cache)
        result.update(
            memoryTotal=total * 1024,
            memoryAvail=available * 1024,
            memory=100 * (total - available) / total,
            online=True,
        )
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
        interfaces = discover_interfaces(cfg)
        result["interfaces"] = interfaces
        idx = cfg["iface"] or next((x["index"] for x in interfaces if x["up"]), "")
        if not idx:
            return result
        result["interfaceIndex"] = idx
        rx, tx = sn("1.3.6.1.2.1.31.1.1.1.6." + idx), sn(
            "1.3.6.1.2.1.31.1.1.1.10." + idx
        )
        now = time.monotonic()
        if previous and rx >= previous[0] and tx >= previous[1]:
            result.update(
                rx=(rx - previous[0]) / (now - previous[2]),
                tx=(tx - previous[1]) / (now - previous[2]),
            )
        previous = rx, tx, now
        result["interface"] = sn("1.3.6.1.2.1.2.2.1.2." + idx)
    except (OSError, ValueError, ZeroDivisionError):
        pass
    return result


def poll():
    global previous, last_ok
    seen = -1
    while True:
        with lock:
            cfg, rev = copy.deepcopy(config), revision
        if rev != seen:
            previous = None
            seen = rev
        result = sample(cfg)
        with lock:
            if rev == revision:
                state.update(result, mode=cfg["mode"])
                if result["online"]:
                    last_ok = time.monotonic()
                state["history"] = (state["history"] + [[result["rx"], result["tx"]]])[
                    -30:
                ]
        for _ in range(cfg["interval"] * 10):
            time.sleep(0.1)
            with lock:
                if revision != rev:
                    break


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
        "SNMP",
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
                    int(w["valid"] and not w["error"]),
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
                        disk["name"].replace(" ", "_"),
                        disk["model"].replace(" ", "_"),
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
        payload += f"{volume['name']} 1 {volume['total']} {volume['used']}\n"
    payload += f"{cfg['language']} {cfg['clockColor']} {data['diskGroup']} {data['volumeGroup']} {int(data.get('resetPending',False))} {cfg['minuteColor']} {cfg['showSeconds']} {(dt.weekday()+1)%7} {int(time.monotonic()*1000/37)%10} {cfg.get('nasBackground','#21455c')}\n"
    text = subprocess.run(
        [
            str(
                ROOT
                / "simulator"
                / ("renderer.exe" if sys.platform == "win32" else "renderer")
            )
        ],
        input=payload.encode("utf8"),
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
            text("Key: {key}").format(key="12345678"),
            text("Open 192.168.4.1"),
        ]
        if mode == "setup"
        else ["127.0.0.1", text("Scan to open settings"), text("Touch to return")]
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

    def signed_in(self):
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
        return True

    def entry(self, setup=False, error=""):
        action = "/wifi/save" if setup else "/login"
        template = (ROOT / "web/access.html").read_text(encoding="utf8")
        for key, value in dict(
            mode="setup" if setup else "login",
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
                k: v for k, v in cfg.items() if k not in ("pass", "admin", "community")
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
        if path.path in ("/i18n.js", "/management.js", "/setup.js", "/access.js"):
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
        length = int(self.headers.get("Content-Length", "0"))
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
        if self.path == "/wifi/save":
            ssid, password = form.get("ssid", ""), form.get("password", "")
            if not 0 < len(ssid.encode()) <= 32 or not 8 <= len(password) <= 63:
                return self.send(dict(error="invalid"), code=400)
            with lock:
                config.update(ssid=ssid, **{"pass": password})
            return self.send(
                {
                    "saved": True,
                    "message": "模拟器不连接实际 Wi-Fi / Simulator does not join Wi-Fi",
                }
            )
        if self.path == "/api/language":
            if form.get("language") not in ("zh", "en"):
                return self.send("Invalid language", "text/plain", 400)
            with lock:
                config["adminLanguage"] = form["language"]
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
        if self.path == "/api/page":
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
                for k in ("community", "pass", "admin"):
                    if form.get(k):
                        new[k] = form[k]
                with lock:
                    if config["alarmTime"] != new["alarmTime"]:
                        alarm_day = ""
                    config.update(new)
                    revision += 1
                    state.update(page=new["home"], history=[], online=False)
                return self.send(
                    '<meta charset="utf-8"><p>'
                    + ("Saved." if new["adminLanguage"] == "en" else "已保存。")
                    + '</p><a href="/manage">MOS2</a>',
                    "text/html; charset=utf-8",
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
