"""Exercise pagination, reset confirmation and limits against an isolated server."""

import importlib.util, sys, threading, json, tempfile, io
from pathlib import Path
from urllib.request import Request, urlopen, build_opener, HTTPCookieProcessor
from http.cookiejar import CookieJar
from urllib.parse import urlencode
from urllib.error import HTTPError
from PIL import Image

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("sim", root / "simulator/server.py")
sim = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sim)
# Fair queue: a rapid worker cannot jump ahead of a waiting collector.
real_snmp = sim.snmp_query
release = threading.Event()
first_started = threading.Event()
starts = []
starts_lock = threading.Lock()


def fake_reply(host, port, community, oid, next_oid=False, timeout=4):
    with starts_lock:
        starts.append((oid, sim.time.monotonic()))
        if len(starts) == 1:
            first_started.set()
    release.wait(2)
    return oid, 1


sim.snmp_query = fake_reply
base_ticket = sim._snmp_ticket
workers = []
try:
    for i in range(4):
        t = threading.Thread(
            target=sim.query, args=("test", 161, "public", str(i)), daemon=True
        )
        t.start()
        workers.append(t)
        deadline = sim.time.monotonic() + 2
        while (
            sim._snmp_ticket < base_ticket + i + 1 and sim.time.monotonic() < deadline
        ):
            sim.time.sleep(0.001)
    assert first_started.wait(2)
    assert len(starts) == 1 and sim._snmp_active == 1
    release.set()
    for t in workers:
        t.join(2)
    assert [v[0] for v in starts] == ["0", "1", "2", "3"]
    assert all(b[1] - a[1] >= 0.49 for a, b in zip(starts, starts[1:]))
    assert sim._snmp_active == 0 and not any(t.is_alive() for t in workers)
finally:
    release.set()
    for t in workers:
        t.join(2)
    sim.snmp_query = real_snmp
print("PASS: FIFO request fairness, one in-flight slot, send spacing, slot release")
# Repeated timeouts open the global circuit without making NAS requests.
def timeout_reply(*args, **kwargs):
    raise TimeoutError("test timeout")
sim.snmp_query = timeout_reply
try:
    for _ in range(3):
        try:
            sim.query("test", 161, "public", "test")
        except TimeoutError:
            pass
    assert sim._snmp_cooldown - sim.time.monotonic() > 59
    assert sim._snmp_active == 0
finally:
    sim.snmp_query = real_snmp
    sim._snmp_failures = 0
    sim._snmp_cooldown = 0
print("PASS: three timeouts pause all requests for sixty seconds")
# Automatic selection must not pick a disconnected Ethernet port or loopback.
original_query = sim.query
interface_rows = {"1": ("lo", 24, 1), "3": ("eth1", 6, 2), "4": ("eth0", 6, 1)}


def interface_query(host, port, community, oid, next_oid=False):
    root = "1.3.6.1.2.1.2.2.1."
    if next_oid:
        start = (
            int(oid.rsplit(".", 1)[1])
            if oid.count(".") == (root + "2.1").count(".")
            else 0
        )
        for index, row in interface_rows.items():
            if int(index) > start:
                return root + "2." + index, row[0]
        return root + "3.1", 24
    column, index = oid[len(root) :].split(".")
    return oid, interface_rows[index][1 if column == "3" else 2]


sim.query = interface_query
ports = sim.discover_interfaces(dict(nas="test", port=161, community="public"))
assert [x["name"] for x in ports] == ["eth1", "eth0"]
assert [x["index"] for x in ports if x["up"]] == ["4"]
sim.query = original_query
sim.interface_cache.clear()
# Unsupported CPU/storage OIDs must not suppress memory or Ethernet sampling.
original_interfaces = sim.discover_interfaces


def partial_query(host, port, community, oid, next_oid=False):
    values = {
        "1.3.6.1.4.1.6574.1.2.0": 55,
        "1.3.6.1.4.1.6574.1.5.1.0": "DS920+",
        "1.3.6.1.2.1.25.1.1.0": 10000,
        "1.3.6.1.4.1.2021.4.5.0": 1000,
        "1.3.6.1.4.1.2021.4.6.0": 600,
        "1.3.6.1.4.1.2021.4.14.0": 100,
        "1.3.6.1.4.1.2021.4.15.0": 100,
        "1.3.6.1.2.1.31.1.1.1.6.4": 1234,
        "1.3.6.1.2.1.31.1.1.1.10.4": 5678,
        "1.3.6.1.2.1.2.2.1.2.4": "eth0",
    }
    if oid not in values:
        raise TimeoutError("Unsupported CPU or storage")
    return oid, values[oid]


sim.query = partial_query
sim.discover_interfaces = lambda cfg: [dict(index="4", name="eth0", up=True)]
partial = sim.sample(sim.config)
assert partial["online"] and partial["cpu"] == -1 and partial["memory"] == 20
assert partial["interface"] == "eth0" and partial["temperature"] == 55
sim.query, sim.discover_interfaces = original_query, original_interfaces
sim.previous = None
# Appearance/language edits keep the acquisition identity; changing NAS clears it.
assert sim.target_key(sim.config) == sim.target_key(
    {**sim.config, "language": "en", "clockColor": "#ff0000"}
)
assert sim.target_key(sim.config) != sim.target_key(
    {**sim.config, "nas": "192.168.1.200"}
)
cached, times = {}, {}
sim.merge_sample(
    cached,
    dict(
        memory=20,
        memoryTotal=1000,
        memoryAvail=800,
        rx=12,
        tx=34,
        interface="eth0",
        online=True,
    ),
    times,
    100,
)
failed = dict(
    memory=-1, memoryTotal=0, memoryAvail=0, rx=-1, tx=-1, interface="", online=False
)
sim.merge_sample(cached, failed, times, 110)
assert cached["memory"] == 20 and cached["rx"] == 12 and not cached["online"]
sim.merge_sample(cached, failed, times, 131)
assert cached["memory"] == -1 and cached["rx"] == -1

# A memory job may only issue its four coherent OIDs; it must not wait on CPU/network.
requested = []


def memory_only(host, port, community, oid, next_oid=False):
    requested.append(oid)
    return partial_query(host, port, community, oid, next_oid)


sim.query = memory_only
assert sim.sample(sim.config, "memory")["memory"] == 20
assert len(requested) == 4 and all("2021.4." in oid for oid in requested)


# Partial memory snapshots must never publish an inaccurate total/free combination.
def incomplete_memory(host, port, community, oid, next_oid=False):
    if oid.endswith(".15.0"):
        raise TimeoutError("cache missing")
    return memory_only(host, port, community, oid, next_oid)


sim.query = incomplete_memory
assert sim.sample(sim.config, "memory")["memory"] == -1
sim.query = original_query
sim.state.update(cpu=7, memory=20, rx=12, tx=34, temperature=51)
sim.collection_stamps.update(cpu=70, memory=125, network=70, temperature=125)
sim.last_ok = 125
sim.expire_collections(131, sim.config)
assert sim.state["cpu"] == -1 and sim.state["rx"] == -1 and sim.state["tx"] == -1
assert (
    sim.state["memory"] == 20 and sim.state["temperature"] == 51 and sim.state["online"]
)
sim.disks = [dict(index="1", status=1, temperature=40, health=1)]
sim.volumes = [dict(index="54", name="/volume1", total=1000, used=0, valid=True)]
sim.table_stamps.update(
    {
        ("disk", "1", "temperature"): 125,
        ("disk", "1", "status"): -7070,
        ("disk", "1", "health"): 125,
        ("volume", "54", "used"): -14270,
    }
)
sim.expire_tables(131)
assert sim.disks[0]["status"] == -1 and sim.disks[0]["temperature"] == 40
assert sim.volumes[0]["used"] is None and not sim.volumes[0]["valid"]
print(
    "PASS: independent coherent memory job, partial batch rejected, per-group and per-field expiry, unknown volume distinct from zero"
)

# A lost table reply resumes at the cursor, rather than repeatedly scanning the root.
original_clock, original_sleep = sim.time.monotonic, sim.time.sleep
old_volumes, old_stamps = sim.volumes, dict(sim.table_stamps)
clock, calls, failed_once = [1000.0], [], [False]
volume_root = "1.3.6.1.2.1.25.2.3.1.3"


def table_query(host, port, community, oid, next_oid=False):
    if next_oid:
        calls.append(oid)
        if oid == volume_root:
            return volume_root + ".1", "/volume1"
        if oid == volume_root + ".1":
            if not failed_once[0]:
                failed_once[0] = True
                raise TimeoutError("One lost table reply")
            return volume_root + ".2", "/volume2"
        return "1.3.6.1.2.1.25.2.3.1.4.1", 1
    column = oid.split(".")[-2]
    return oid, {"4": 1, "5": 100, "6": 20}[column]


def fast_sleep(seconds):
    if len(sim.volumes) == 2 and all(v.get("valid") for v in sim.volumes):
        raise SystemExit()
    clock[0] += 31


sim.query = table_query
sim.time.monotonic = lambda: clock[0]
sim.time.sleep = fast_sleep
old_page = sim.state["page"]
sim.state["page"] = 9  # This regression exercises the active volume page.
sim._startup_key = None
try:
    try:
        sim.table_poll("volume")
    except SystemExit:
        pass
    assert calls.count(volume_root) == 1 and calls.count(volume_root + ".1") == 2
    assert len(sim.volumes) == 2 and all(
        v["valid"] and v["used"] == 20 for v in sim.volumes
    )
finally:
    sim.query = original_query
    sim.time.monotonic, sim.time.sleep = original_clock, original_sleep
    sim.volumes, sim.table_stamps = old_volumes, old_stamps
    sim.state["page"] = old_page
    sim._startup_key = None
print(
    "PASS: interrupted volume discovery resumes cursor, retains completed volume, recovers without restarting table"
)


with tempfile.TemporaryDirectory() as folder:
    sim.album_dir = Path(folder)
    Image.new("RGB", (240, 240), "teal").save(sim.album_dir / "photo-0.jpg")
    sim.config.update(language="en", clockColor="#ff0000")
    sim.disks = [
        dict(name=f"Disk{i}", model="Test", temperature=30, status=1, health=1)
        for i in range(1, 13)
    ]
    sim.volumes = [dict(name=f"/volume{i}", total=1000, used=500) for i in range(1, 9)]
    server = sim.ThreadingHTTPServer(("127.0.0.1", 0), sim.Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    base = "http://127.0.0.1:" + str(server.server_address[1])

    sim.config_path = sim.album_dir.parent / "config.json"
    # Password-only browser login: failed login cannot expose configuration.
    assert 'data-mode="login"' in urlopen(base + "/manage").read().decode()
    json.load(
        urlopen(
            Request(
                base + "/api/simulator/page",
                data=urlencode(dict(token=sim.token, page=0)).encode(),
            )
        )
    )
    assert sim.state["page"] == 0
    browser = build_opener(HTTPCookieProcessor(CookieJar()))
    failed = (
        browser.open(
            Request(
                base + "/login",
                data=urlencode(dict(token=sim.token, password="wrong")).encode(),
            )
        )
        .read()
        .decode()
    )
    assert 'data-error="wrong"' in failed and "management.js" not in failed
    logged = (
        browser.open(
            Request(
                base + "/login",
                data=urlencode(dict(token=sim.token, password="12345678")).encode(),
            )
        )
        .read()
        .decode()
    )
    assert 'data-mode="change"' in logged and "management.js" not in logged
    try:
        browser.open(
            Request(
                base + "/api/language",
                data=urlencode(dict(token=sim.token, language="en")).encode(),
            )
        )
        raise AssertionError("Default session reached a privileged endpoint")
    except HTTPError as e:
        assert e.code == 403
    for password, confirm in [
        ("12345678", "12345678"),
        ("short", "short"),
        ("NewTestPassword", "different"),
    ]:
        try:
            browser.open(
                Request(
                    base + "/password/change",
                    data=urlencode(
                        dict(
                            token=sim.token,
                            newPassword=password,
                            confirmPassword=confirm,
                        )
                    ).encode(),
                )
            )
            raise AssertionError("Invalid change accepted")
        except HTTPError as e:
            assert e.code == 400
    assert json.load(
        browser.open(
            Request(
                base + "/password/change",
                data=urlencode(
                    dict(
                        token=sim.token,
                        newPassword="NewTestPassword",
                        confirmPassword="NewTestPassword",
                    )
                ).encode(),
            )
        )
    )["changed"]
    assert json.loads(sim.config_path.read_text())["admin"] == "NewTestPassword"
    sim.config["admin"] = "12345678"
    sim.load_config(sim.config_path)
    assert sim.config["admin"] == "NewTestPassword"
    labels = [
        op[-1] for op in sim.qr_commands("management", sim.config) if op[0] == "text"
    ]
    assert "Default password changed" in labels and not any(
        "12345678" in v for v in labels
    )
    setup_labels = [
        op[-1] for op in sim.qr_commands("setup", sim.config) if op[0] == "text"
    ]
    assert any(
        "12345678" in v for v in setup_labels
    ), "Wi-Fi AP password must stay independent"

    assert 'data-mode="login"' in browser.open(base + "/manage").read().decode()
    logged = (
        browser.open(
            Request(
                base + "/login",
                data=urlencode(
                    dict(token=sim.token, password="NewTestPassword")
                ).encode(),
            )
        )
        .read()
        .decode()
    )
    assert "management.js" in logged
    assert 'data-mode="login"' not in browser.open(base + "/manage").read().decode()
    with sim.lock:
        for session in sim.sessions:
            sim.sessions[session] -= 1801
    assert 'data-mode="login"' in browser.open(base + "/manage").read().decode()
    setup = urlopen(base + "/setup").read().decode()
    assert 'name="ssid"' in setup and 'name="nas"' not in setup
    assert "setup.js" in setup
    scans = json.load(urlopen(base + "/api/wifi/scan"))
    assert not scans["scanning"] and scans["networks"][0]["ssid"] == "Demo Wi-Fi"

    def post(path, **kw):
        return json.load(
            urlopen(
                Request(
                    base + path, data=urlencode(dict(token=sim.token, **kw)).encode()
                )
            )
        )

    def get(path):
        return json.load(urlopen(base + path))

    saved_uptime = sim.state["uptime"]
    saved_stamp = sim.collection_stamps.get("uptime")
    sim.state["uptime"] = 1000
    sim.collection_stamps["uptime"] = sim.time.monotonic() - 12
    assert get("/api/status")["uptime"] >= 1012
    assert sim.state["uptime"] == 1000  # Display estimation does not alter the NAS baseline.
    sim.state["uptime"] = saved_uptime
    if saved_stamp is None:
        sim.collection_stamps.pop("uptime", None)
    else:
        sim.collection_stamps["uptime"] = saved_stamp
    print("PASS: uptime advances locally without a new SNMP sample")

    try:
        post("/wifi/save", ssid="Example", password="short")
        raise AssertionError("Invalid Wi-Fi settings accepted")
    except HTTPError as error:
        assert error.code == 400 and json.load(error)["error"] == "invalid"
    assert post("/wifi/save", ssid="Example", password="TestOnly8")["saved"]

    # A v3 credential change selects a new transport/cache identity, while UI
    # preferences never alter it. Secret values are not embedded in the HTML.
    old_config = sim.config.copy()
    settings = sim.config.copy()
    settings.update(
        token=sim.token,
        snmpVersion="3",
        snmpUser="test",
        snmpPassword="TestOnly8",
        snmpAuth="MD5",
    )

    def save_settings(values):
        return urlopen(Request(base + "/save", data=urlencode(values).encode()))

    save_settings(settings).close()
    assert (
        sim.config["snmpVersion"] == "3" and sim.config["snmpPassword"] == "TestOnly8"
    )
    settings.update(snmpPrivacy="DES", snmpPrivacyPassword="DifferentPrivatePassword")
    save_settings(settings).close()
    assert (
        sim.config["snmpPrivacy"] == "DES"
        and sim.config["snmpPrivacyPassword"] == "DifferentPrivatePassword"
    )
    browser.open(
        Request(
            base + "/login",
            data=urlencode(
                dict(token=sim.token, password=sim.config["admin"])
            ).encode(),
        )
    ).read()
    public_html = browser.open(base + "/manage").read().decode()
    assert "DifferentPrivatePassword" not in public_html
    settings["snmpPrivacyPassword"] = ""
    save_settings(settings).close()
    assert sim.config["snmpPrivacyPassword"] == "DifferentPrivatePassword"
    key = sim.target_key(sim.config)
    assert key != sim.target_key(old_config)
    changed = dict(sim.config, language="en", brightness=50)
    assert key == sim.target_key(changed)
    settings["snmpPassword"] = ""
    sim.state.update(history=[[10, 20], [15, 25]], online=True)
    save_settings(settings).close()
    assert sim.state["history"] == [[10, 20], [15, 25]] and sim.state["online"]

    assert sim.config["snmpPassword"] == "TestOnly8"
    for field, value in (
        ("snmpAuth", "SHA256"),
        ("snmpUser", ""),
        ("snmpPassword", "short"),
    ):
        invalid = dict(settings, **{field: value})
        try:
            save_settings(invalid).close()
            raise AssertionError("Invalid v3 settings accepted")
        except HTTPError as error:
            assert error.code == 400
    # All four supported authentication/privacy combinations round-trip settings.
    for auth in ("MD5", "SHA"):
        for privacy in ("DES", "AES"):
            supported = dict(settings, snmpAuth=auth, snmpPrivacy=privacy)
            save_settings(supported).close()
            assert (
                sim.config["snmpAuth"] == auth and sim.config["snmpPrivacy"] == privacy
            )
    browser.open(
        Request(
            base + "/login",
            data=urlencode(
                dict(token=sim.token, password=sim.config["admin"])
            ).encode(),
        )
    ).read()
    signed_out = json.load(
        browser.open(
            Request(base + "/logout", data=urlencode(dict(token=sim.token)).encode())
        )
    )
    assert signed_out["signedOut"]
    assert 'data-mode="login"' in browser.open(base + "/manage").read().decode()
    assert not sim.sessions
    browser.open(
        Request(
            base + "/login",
            data=urlencode(
                dict(token=sim.token, password=sim.config["admin"])
            ).encode(),
        )
    ).read()
    # Unauthenticated setup exposes no NAS credentials.
    assert "TestOnly8" not in urlopen(base + "/setup").read().decode()
    sim.config.clear()
    sim.config.update(old_config)
    print(
        "PASS: v3 settings validation, blank password retention, transport identity and UI isolation"
    )

    post("/api/page", page=8)
    for group, name in [(0, "Disk1"), (1, "Disk5"), (2, "Disk9"), (0, "Disk1")]:
        d = get("/api/display")
        assert d["status"]["page"] == 8 and d["status"]["diskGroup"] == group
        assert name in [c[6] for c in d["commands"] if c[0] == "text"]
        post("/api/touch", action="double")
    post("/api/page", page=9)
    for group, name in [
        (0, "/volume1"),
        (1, "/volume4"),
        (2, "/volume7"),
        (0, "/volume1"),
    ]:
        d = get("/api/display")
        assert d["status"]["volumeGroup"] == group
        assert name in [c[6] for c in d["commands"] if c[0] == "text"]
        post("/api/touch", action="double")
    post("/api/touch", action="tap")
    assert get("/api/status")["page"] == 4
    post("/api/touch", action="five")
    assert get("/api/status")["uiMode"] == "management"
    assert get("/api/display")["commands"][1][6] == "MANAGEMENT"
    post("/api/touch", action="eight")
    assert sim.reset_pending and sim.config["language"] == "en"
    post("/api/touch", action="triple")
    assert (
        not sim.reset_pending
        and sim.config["language"] == "zh"
        and get("/api/status")["albumCount"] == 0
    )
    Image.new("RGB", (240, 240), "teal").save(sim.album_dir / "photo-0.jpg")
    # Language preferences are independent.
    sim.config["admin"] = "NewTestPassword"
    post("/api/language", language="en")
    assert sim.config["language"] == "zh" and sim.config["adminLanguage"] == "en"
    # Fixed pictures do not advance with time; double click advances and wraps.
    Image.new("RGB", (240, 240), "orange").save(sim.album_dir / "photo-1.jpg")
    sim.config["albumFixed"] = 1
    sim.album_position = 0
    sim.album_at -= 1000
    post("/api/page", page=7)
    assert get("/api/status")["albumSlot"] == 0
    post("/api/touch", count=2)
    assert get("/api/status")["albumSlot"] == 1
    post("/api/touch", count=2)
    assert get("/api/status")["albumSlot"] == 0
    # Two separate colors reach the shared renderer; desktop headers are absent.
    sim.config.update(clockColor="#ff0000", minuteColor="#00ff00", language="en")
    d = get("/api/display?page=6")
    assert any(c[-1] == 63488 for c in d["commands"] if c[0] in ("rect", "triangle"))
    assert any(c[-1] == 2016 for c in d["commands"] if c[0] in ("rect", "triangle"))
    assert "DESKTOP CLOCK" not in [c[6] for c in d["commands"] if c[0] == "text"]
    assert "WEATHER / CLOCK" not in [
        c[6] for c in get("/api/display?page=5")["commands"] if c[0] == "text"
    ]
    # Alarm icon pairs with clock alarm time; weather keeps it above seconds.
    sim.config["alarmEnabled"] = 1
    for page in (5, 6):
        for seconds in (0, 1):
            sim.config["showSeconds"] = seconds
            commands = get(f"/api/display?page={page}")["commands"]
            assert any(
                c[0] == "circle" and c[2:4] == [96 if page == 5 else 198, 8]
                for c in commands
            )
            if page == 6:
                assert not any(c[:4] == ["circle", 202, 96, 8] for c in commands)
    sim.config["alarmEnabled"] = 0
    assert not any(
        c[0] == "circle" and c[2:4] == [198, 8]
        for c in get("/api/display?page=6")["commands"]
    )
    sim.config["showSeconds"] = 1
    (sim.album_dir / "photo-1.jpg").unlink()
    Image.new("RGB", (240, 240), "orange").save(sim.album_dir / "photo-11.jpg")
    nonce = post("/api/reset/request")["nonce"]
    try:
        post("/api/reset/confirm", nonce="wrong")
        raise AssertionError()
    except HTTPError as e:
        assert e.code == 403
    post("/api/reset/confirm", nonce=nonce)
    sim.config["admin"] = "NewTestPassword"
    assert (
        get("/api/status")["albumCount"] == 0
        and not list(sim.album_dir.glob("photo-*"))
        and sim.config["nas"] == "192.168.1.100"
        and not sim.config["ssid"]
        and sim.alarm["state"] == 0
    )
    Image.new("RGB", (240, 240), "teal").save(sim.album_dir / "photo-0.jpg")
    # The image body exceeds the upload limit and must not replace the old photo.
    before = (sim.album_dir / "photo-0.jpg").read_bytes()
    body = b"x" * 55001
    try:
        urlopen(
            Request(
                base + "/api/album/upload?slot=0",
                data=body,
                headers={"X-CSRF-Token": sim.token},
            )
        )
        raise AssertionError()
    except HTTPError as e:
        assert e.code == 413
    assert before == (sim.album_dir / "photo-0.jpg").read_bytes()
    # Add automatically uses the next empty slot; seven pictures reject the eighth.
    import uuid

    jpeg = io.BytesIO()
    Image.new("RGB", (240, 240), "blue").save(jpeg, format="JPEG")
    jpeg = jpeg.getvalue()
    boundary = "photo-test"
    body = (
        (
            "--"
            + boundary
            + '\r\nContent-Disposition: form-data; name="file"; filename="photo.jpg"\r\nContent-Type: image/jpeg\r\n\r\n'
        ).encode()
        + jpeg
        + ("\r\n--" + boundary + "--\r\n").encode()
    )
    headers = {
        "X-CSRF-Token": sim.token,
        "Content-Type": "multipart/form-data; boundary=" + boundary,
    }
    for _ in range(6):
        json.load(
            urlopen(
                Request(base + "/api/album/upload?mode=add", data=body, headers=headers)
            )
        )
    assert (
        len(get("/api/album")) == 7
        and (sim.album_dir / "photo-0.jpg").read_bytes() == before
    )
    try:
        urlopen(
            Request(base + "/api/album/upload?mode=add", data=body, headers=headers)
        )
        raise AssertionError()
    except HTTPError as e:
        assert e.code == 400
    assert len(get("/api/album")) == 7
    # Saving via fetch must return a message, never a full HTML document.
    for language, expected in (("zh", "已保存。"), ("en", "Saved.")):
        post("/api/language", language=language)
        values = {**sim.config, "token": sim.token}
        response = browser.open(
            Request(base + "/save", data=urlencode(values).encode())
        )
        assert response.headers.get_content_type() == "text/plain"
        assert response.read().decode() == expected

    # OTA simulation validates the same layout/resource contract as hardware.
    import zlib

    font = (root / "data/fonts/ui04.bin").read_bytes()
    firmware = bytearray(b"\xff" * 5000)
    firmware[0] = 0xE9
    firmware[4090:4106] = b"MOS2OTA1" + (0x200000).to_bytes(4, "little") + font[12:16]
    browser.open(
        Request(
            base + "/login",
            data=urlencode(
                dict(token=sim.token, password=sim.config["admin"])
            ).encode(),
        )
    ).read()

    def upgrade(
        content, keep="1", opener=browser, csrf=sim.token, checksum=None, size=None
    ):
        boundary = "ota-test"
        body = (
            b'--ota-test\r\nContent-Disposition: form-data; name="firmware"; filename="test.bin"\r\nContent-Type: application/octet-stream\r\n\r\n'
            + bytes(content)
            + b"\r\n--ota-test--\r\n"
        )
        args = dict(
            size=len(content) if size is None else size,
            crc=f"{zlib.crc32(content) if checksum is None else checksum:08x}",
        )
        if keep is not None:
            args["keep"] = keep
        return json.load(
            opener.open(
                Request(
                    base + "/api/firmware?" + urlencode(args),
                    data=body,
                    headers={
                        "X-CSRF-Token": csrf,
                        "Content-Type": "multipart/form-data; boundary=" + boundary,
                    },
                )
            )
        )

    resource_before = font
    sim.config["ssid"] = "Preserved Wi-Fi"
    photos_before = {p.name: p.read_bytes() for p in sim.album_dir.glob("*.jpg")}
    result = upgrade(firmware, keep=None)
    assert result["preserved"] and result["simulated"]
    assert sim.config["ssid"] == "Preserved Wi-Fi"
    for kwargs, code in [
        ({"opener": build_opener()}, 401),
        ({"csrf": "wrong"}, 403),
        ({"checksum": 0}, 400),
        ({"size": 5001}, 400),
        ({"size": 1048577}, 400),
    ]:
        try:
            upgrade(firmware, keep="0", **kwargs)
            raise AssertionError("Invalid update accepted")
        except HTTPError as error:
            assert error.code == code
    invalid = bytearray(firmware)
    invalid[4090] = 0
    try:
        upgrade(invalid, keep="0")
        raise AssertionError("Wrong resource contract accepted")
    except HTTPError as error:
        assert error.code == 400
    assert {
        p.name: p.read_bytes() for p in sim.album_dir.glob("*.jpg")
    } == photos_before
    assert sim.config["ssid"] == "Preserved Wi-Fi"
    assert not upgrade(firmware, keep="0")["preserved"]
    assert sim.config == sim.defaults and not list(sim.album_dir.glob("*.jpg"))
    assert (root / "data/fonts/ui04.bin").read_bytes() == resource_before
    print(
        "PASS: OTA authentication, CRC, size, compatibility, default keep, clear user data, retain font resource"
    )
    server.shutdown()
print(
    "PASS: 12 disks / 8 volumes manual groups, single next, centered QR, eight + triple reset, nonce protection, factory reset clears photos, oversized upload rejected"
)

assert sim.startup_delays(6)["cpu"] == 20
assert all(sim.startup_delays(0)[g] == 0 for g in ("cpu", "memory", "temperature", "network"))
assert sim.startup_delays(1)["interfaces"] == 0
assert sim.startup_delays(8)["disk"] == 0 and sim.startup_delays(9)["volume"] == 0
for page in range(10):
    assert max(sim.startup_delays(page).values()) <= 70
print("PASS: page-aware startup priorities and bounded background staggering")

old_home, old_page, old_path = sim.config["home"], sim.state["page"], sim.config_path
with tempfile.TemporaryDirectory() as folder:
    path = Path(folder) / "startup.json"
    path.write_text(json.dumps({"home": 2}))
    sim.load_config(path)
    assert sim.state["page"] == 2
sim.config["home"], sim.state["page"], sim.config_path = old_home, old_page, old_path
print("PASS: saved startup page drives simulator display and acquisition priority")

retry_calls = []
def lost_once(host, port, community, oid, next_oid=False):
    retry_calls.append(oid)
    if len(retry_calls) == 1: raise TimeoutError("one lost reply")
    return oid, 55
sim.query = lost_once
assert sim.sample(sim.config, "temperature")["temperature"] == 55
assert len(retry_calls) == 2
sim.query = original_query
print("PASS: isolated OID timeout retries once without discarding other fields")
