"""USM crypto vectors, strict response verification and local test-agent round trips."""

import sys, threading, socket, hashlib, hmac
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import snmp_v3 as v3
from snmp_probe import tlv, integer, oid

credentials = v3.Credentials("test", "maplesyrup")
s = v3.Session(credentials)
assert s.ku.hex() == "9faf3283884e92834ebc9847d8edd963"
s.sync(dict(engine=bytes(11) + b"\x02", boots=1, time=12))
assert s.key.hex() == "526f5eed9fcce26f8964c2930787d82b"
packet = s.packet(7, 3, "1.3.6.1.2.1.1.3.0")
m = v3.parse(packet)
s.verify(packet, m)
for i in range(len(packet)):
    try:
        v3.parse(packet[:i])
        raise AssertionError("Accepted truncation")
    except ValueError:
        pass
try:
    s.verify(packet[:-1] + bytes([packet[-1] ^ 1]), m)
    raise AssertionError("Accepted tamper")
except ValueError:
    pass
assert "maplesyrup" not in repr(credentials)


def response(ident, request, name, discovery=False):
    pdu = tlv(
        0xA8 if discovery else 0xA2,
        integer(request)
        + integer(0)
        + integer(0)
        + tlv(48, tlv(48, oid(name) + integer(42))),
    )
    scoped = tlv(48, tlv(4, s.engine) + tlv(4, b"") + pdu)
    salt = b""
    if not discovery and s.credentials.privacy == "DES":
        salt = bytes.fromhex("0000000100000007")
        iv = bytes(a ^ b for a, b in zip(s.privacy_key[8:16], salt))
        scoped = tlv(
            4,
            v3.DES.new(s.privacy_key[:8], v3.DES.MODE_CBC, iv).encrypt(
                scoped + bytes((-len(scoped)) % 8)
            ),
        )
    if not discovery and s.credentials.privacy == "AES":
        salt = bytes.fromhex("0123456789abcdef")
        iv = s.boots.to_bytes(4, "big") + s.clock().to_bytes(4, "big") + salt
        scoped = tlv(
            4,
            v3.AES.new(
                s.privacy_key[:16], v3.AES.MODE_CFB, iv, segment_size=128
            ).encrypt(scoped),
        )
    header = tlv(
        48,
        integer(ident)
        + integer(1500)
        + tlv(4, bytes([0 if discovery else (3 if salt else 1)]))
        + integer(3),
    )
    security = tlv(
        48,
        tlv(4, s.engine)
        + integer(s.boots)
        + integer(s.clock())
        + tlv(4, b"" if discovery else b"test")
        + tlv(4, b"" if discovery else bytes(12))
        + tlv(4, salt),
    )
    out = tlv(
        48,
        integer(3) + header + tlv(4, security) + scoped,
    )
    if not discovery:
        auth = v3.parse(out)["auth"]
        out = (
            out[: auth[1]] + hmac.digest(s.key, out, s.hash_name)[:12] + out[auth[2] :]
        )
    return out


udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp.bind(("127.0.0.1", 0))
udp.settimeout(5)
errors = []


def agent():
    try:
        for discovery in (True, False, False):
            packet, addr = udp.recvfrom(1500)
            m = v3.parse(packet)
            rid, error, index, bindings = v3.children(packet, m["pdu"])
            request = v3.number(packet, rid)
            if not discovery:
                s.verify(packet, m)
            name = "1.3.6.1.6.3.15.1.1.4.0" if discovery else "1.3.6.1.2.1.1.3.0"
            udp.sendto(response(m["id"], request, name, discovery), addr)
    except Exception as e:
        errors.append(e)


thread = threading.Thread(target=agent)
thread.start()
client = v3.Session(credentials)
assert client.query("127.0.0.1", udp.getsockname()[1], "1.3.6.1.2.1.1.3.0") == (
    "1.3.6.1.2.1.1.3.0",
    42,
)
assert client.query("127.0.0.1", udp.getsockname()[1], "1.3.6.1.2.1.1.3.0")[1] == 42
thread.join()
udp.close()
assert not errors, errors
print("Python SNMP v3 discovery/authentication/session reuse passed.")

s = v3.Session(
    v3.Credentials("test", "maplesyrup", "MD5", "DES", "privateTestPassword")
)
s.sync(dict(engine=bytes(11) + b"\x02", boots=1, time=12))
assert "privateTestPassword" not in repr(s.credentials)
packet = s.packet(11, 9, "1.3.6.1.2.1.1.3.0")
m = v3.parse(packet)
s.verify(packet, m)
plain = s.unwrap(packet, m)
assert m["pdu"][0] == 0xA0
first_salt = v3.parse(packet)["privacy"]
assert v3.parse(s.packet(12, 10, "1.3.6.1.2.1.1.3.0"))["privacy"] != first_salt
packet = response(11, 9, "1.3.6.1.2.1.1.3.0")
m = v3.parse(packet)
plain = s.unwrap(packet, m)
assert v3.value(plain, m, 9)[1] == 42
try:
    s.unwrap(packet[:-1] + bytes([packet[-1] ^ 1]), v3.parse(packet))
    raise AssertionError("Ciphertext tampering accepted")
except ValueError as e:
    assert "authentication" in str(e)
no_priv = v3.Session(credentials)
no_priv.sync(dict(engine=s.engine, boots=1, time=12))
packet = no_priv.packet(1, 1, "1.3.6.1.2.1.1.3.0")
try:
    s.unwrap(packet, v3.parse(packet))
    raise AssertionError("Privacy downgrade accepted")
except ValueError:
    pass
key = bytes.fromhex("133457799bbcdff1")
block = bytes.fromhex("0123456789abcdef")
assert (
    v3.DES.new(key, v3.DES.MODE_CBC, bytes(8)).encrypt(block).hex()
    == "85e813540f0ab405"
)
print(
    "PASS: DES known vector, separate privacy key, encrypted response, unique salt, authenticate-before-decrypt and downgrade rejection"
)

for auth in ("MD5", "SHA"):
    for privacy in ("DES", "AES"):
        s = v3.Session(
            v3.Credentials("test", "maplesyrup", auth, privacy, "privateTestPassword")
        )
        private = b"privateTestPassword"
        expected_hash = hashlib.sha1 if auth == "SHA" else hashlib.md5
        expected_ku = expected_hash(
            (private * (1048576 // len(private) + 1))[:1048576]
        ).digest()
        assert s.privacy_ku == expected_ku
        s.sync(dict(engine=bytes(11) + b"\x02", boots=1, time=12))
        assert (
            s.privacy_key
            == expected_hash(expected_ku + s.engine + expected_ku).digest()
        )
        if auth == "SHA":
            assert s.ku.hex() == "9fb5cc0381497b3793528939ff788d5d79145211"
            assert s.key.hex() == "6695febc9288e36282235fc7151f128497b38f3f"
        packet = s.packet(11, 9, "1.3.6.1.2.1.1.3.0")
        m = v3.parse(packet)
        s.unwrap(packet, m)
        assert m["pdu"][0] == 0xA0
        packet = response(11, 9, "1.3.6.1.2.1.1.3.0")
        m = v3.parse(packet)
        plain = s.unwrap(packet, m)
        assert v3.value(plain, m, 9)[1] == 42
        first = v3.parse(packet)["privacy"]
        assert v3.parse(s.packet(12, 10, "1.3.6.1.2.1.1.3.0"))["privacy"] != first
print("PASS: SHA localization and MD5/SHA with DES/AES encrypted round trips")
