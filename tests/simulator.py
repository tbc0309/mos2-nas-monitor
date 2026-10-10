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
# Automatic selection must not pick a disconnected Ethernet port or loopback.
original_query = sim.query
interface_rows = {"1": ("lo", 24, 1), "3": ("eth1", 6, 2), "4": ("eth0", 6, 1)}
def interface_query(host, port, community, oid, next=False):
    root = "1.3.6.1.2.1.2.2.1."
    if next:
        start = int(oid.rsplit(".", 1)[1]) if oid.count(".") == (root + "2.1").count(".") else 0
        for index, row in interface_rows.items():
            if int(index) > start:
                return root + "2." + index, row[0]
        return root + "3.1", 24
    column, index = oid[len(root):].split(".")
    return oid, interface_rows[index][1 if column == "3" else 2]
sim.query = interface_query
ports = sim.discover_interfaces(dict(nas="test", port=161, community="public"))
assert [x["name"] for x in ports] == ["eth1", "eth0"]
assert [x["index"] for x in ports if x["up"]] == ["4"]
sim.query = original_query
sim.interface_cache.clear()

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

    # Password-only browser login: failed login cannot expose configuration.
    assert 'data-mode="login"' in urlopen(base + "/manage").read().decode()
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

    try:
        post("/wifi/save", ssid="Example", password="short")
        raise AssertionError("Invalid Wi-Fi settings accepted")
    except HTTPError as error:
        assert error.code == 400 and json.load(error)["error"] == "invalid"
    assert post("/wifi/save", ssid="Example", password="TestOnly8")["saved"]

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
    server.shutdown()
print(
    "PASS: 12 disks / 8 volumes manual groups, single next, centered QR, eight + triple reset, nonce protection, factory reset clears photos, oversized upload rejected"
)
