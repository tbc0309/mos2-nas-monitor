"""SNMPv3 USM MD5/SHA-1 authentication and DES/AES-128 privacy.

The session is shared across collectors; authentication covers the whole BER
message and engine discovery never publishes a monitoring value.
"""

from dataclasses import dataclass, field
import hashlib, hmac, secrets, socket, threading, time
from Crypto.Cipher import DES, AES
from snmp_probe import tlv, integer, oid, oid_text


@dataclass(frozen=True)
class Credentials:
    user: str
    password: str = field(repr=False)
    protocol: str = "MD5"
    privacy: str = "none"
    privacy_password: str = field(default="", repr=False)


def node(b, p, limit=None):
    limit = len(b) if limit is None else limit
    if p + 2 > limit:
        raise ValueError("Truncated v3 BER")
    tag, size = b[p : p + 2]
    p += 2
    if size & 128:
        count = size & 127
        if not count or count > 2 or p + count > limit:
            raise ValueError("Invalid v3 BER length")
        size = int.from_bytes(b[p : p + count], "big")
        p += count
    if p + size > limit:
        raise ValueError("Truncated v3 payload")
    return tag, p, p + size


def children(b, n):
    p, end = n[1:]
    out = []
    while p < end:
        child = node(b, p, end)
        out.append(child)
        p = child[2]
    return out


def content(b, n):
    return b[n[1] : n[2]]


def number(b, n):
    if n[0] != 2 or not 1 <= n[2] - n[1] <= 4:
        raise ValueError("Invalid v3 integer")
    return int.from_bytes(content(b, n), "big", signed=True)


def parse(b):
    outer = node(b, 0)
    if outer[0] != 48 or outer[2] != len(b):
        raise ValueError("Invalid v3 envelope")
    version, header, security, scoped = children(b, outer)
    if (
        number(b, version) != 3
        or header[0] != 48
        or security[0] != 4
        or scoped[0] not in (48, 4)
    ):
        raise ValueError("Unsupported v3 security level")
    ident, maximum, flags, model = children(b, header)
    if flags[0] != 4 or flags[2] - flags[1] != 1 or number(b, model) != 3:
        raise ValueError("Invalid v3 header")
    flag = b[flags[1]]
    if flag & ~7 or (flag & 2 and not flag & 1):
        raise ValueError("Invalid security flags")
    seq = node(b, security[1], security[2])
    if seq[0] != 48 or seq[2] != security[2]:
        raise ValueError("Invalid USM")
    engine, boots, clock, user, auth, privacy = children(b, seq)
    if (
        any(n[0] != 4 for n in (engine, user, auth, privacy))
        or privacy[2] - privacy[1] != (8 if flag & 2 else 0)
        or scoped[0] != (4 if flag & 2 else 48)
    ):
        raise ValueError("Invalid USM fields")
    pdu = None
    if not flag & 2:
        pdu = scoped_pdu(b, scoped, content(b, engine))
    return dict(
        id=number(b, ident),
        flags=flag,
        engine=content(b, engine),
        boots=number(b, boots),
        time=number(b, clock),
        user=content(b, user),
        auth=auth,
        pdu=pdu,
        scoped=scoped,
        privacy=content(b, privacy),
    )


def scoped_pdu(b, scoped, engine):
    context, context_name, pdu = children(b, scoped)
    if (
        context[0] != 4
        or content(b, context) != engine
        or context_name[0] != 4
        or content(b, context_name)
    ):
        raise ValueError("Invalid v3 context")
    return pdu


def value(b, m, ident, report=False):
    pdu = m["pdu"]
    if pdu[0] != (0xA8 if report else 0xA2):
        raise ValueError("Not a v3 response")
    request, error, index, bindings = children(b, pdu)
    if number(b, request) != ident or number(b, error):
        raise ValueError("SNMP request ID or error")
    if bindings[0] != 48:
        raise ValueError("Invalid v3 varbind list")
    entries = children(b, bindings)
    if len(entries) != 1 or entries[0][0] != 48:
        raise ValueError("Invalid v3 varbind")
    binding = entries[0]
    name, val = children(b, binding)
    if (
        name[0] != 6
        or name[1] == name[2]
        or val[0] not in (2, 4, 0x41, 0x42, 0x43, 0x46)
    ):
        raise ValueError("OID unavailable")
    data = content(b, val)
    if val[0] != 4 and (not data or len(data) > 9 or (len(data) == 9 and data[0] != 0)):
        raise ValueError("Invalid v3 counter")
    return oid_text(content(b, name)), (
        data.decode("utf8", errors="replace")
        if val[0] == 4
        else int.from_bytes(data, "big", signed=val[0] == 2)
    )


class Session:
    def __init__(self, credentials):
        if (
            credentials.protocol not in ("MD5", "SHA")
            or not 1 <= len(credentials.user.encode()) <= 32
            or not 8 <= len(credentials.password.encode()) <= 64
            or credentials.privacy not in ("none", "DES", "AES")
            or (
                credentials.privacy != "none"
                and not 8 <= len(credentials.privacy_password.encode()) <= 64
            )
        ):
            raise ValueError("Invalid v3 credentials")
        self.credentials = credentials
        self.hash_name = "sha1" if credentials.protocol == "SHA" else "md5"
        self.hash = hashlib.sha1 if credentials.protocol == "SHA" else hashlib.md5
        password = credentials.password.encode()
        self.ku = self.hash(
            (password * (1048576 // len(password) + 1))[:1048576]
        ).digest()
        private = (
            credentials.privacy_password.encode()
            if credentials.privacy != "none"
            else password
        )
        self.privacy_ku = self.hash(
            (private * (1048576 // len(private) + 1))[:1048576]
        ).digest()
        self.salt = (
            secrets.randbits(64)
            if credentials.privacy == "AES"
            else secrets.randbits(32)
        )
        self.engine = b""
        self.key = b""
        self.boots = self.time = self.synced = 0
        self.lock = threading.Lock()
        self.failures = 0
        self.last_response = 0.0

    def sync(self, m):
        if (
            not 5 <= len(m["engine"]) <= 32
            or not 0 <= m["boots"] < 0x7FFFFFFF
            or not 0 <= m["time"] < 0x7FFFFFFF
        ):
            raise ValueError("Invalid engine identity/time")
        self.engine = m["engine"]
        self.boots = m["boots"]
        self.time = m["time"]
        self.synced = time.monotonic()
        self.key = self.hash(self.ku + self.engine + self.ku).digest()
        self.privacy_key = self.hash(
            self.privacy_ku + self.engine + self.privacy_ku
        ).digest()

    def clock(self):
        return self.time + int(time.monotonic() - self.synced)

    def packet(self, ident, request, name, next_oid=False, discovery=False):
        message_time = self.clock()
        pdu = tlv(
            0xA1 if next_oid else 0xA0,
            integer(request)
            + integer(0)
            + integer(0)
            + tlv(48, tlv(48, oid(name) + tlv(5, b""))),
        )
        scoped = tlv(48, tlv(4, b"" if discovery else self.engine) + tlv(4, b"") + pdu)
        privacy = b""
        if not discovery and self.credentials.privacy != "none":
            with self.lock:
                if self.credentials.privacy == "AES":
                    privacy = self.salt.to_bytes(8, "big")
                    self.salt = (self.salt + 1) & 0xFFFFFFFFFFFFFFFF
                else:
                    privacy = self.boots.to_bytes(4, "big") + self.salt.to_bytes(
                        4, "big"
                    )
                    self.salt = (self.salt + 1) & 0xFFFFFFFF
            if self.credentials.privacy == "AES":
                iv = (
                    self.boots.to_bytes(4, "big")
                    + message_time.to_bytes(4, "big")
                    + privacy
                )
                cipher = AES.new(
                    self.privacy_key[:16], AES.MODE_CFB, iv, segment_size=128
                )
                scoped = tlv(4, cipher.encrypt(scoped))
            else:
                iv = bytes(a ^ b for a, b in zip(self.privacy_key[8:16], privacy))
                scoped = tlv(
                    4,
                    DES.new(self.privacy_key[:8], DES.MODE_CBC, iv).encrypt(
                        scoped + bytes((-len(scoped)) % 8)
                    ),
                )
        security = tlv(
            48,
            tlv(4, b"" if discovery else self.engine)
            + integer(0 if discovery else self.boots)
            + integer(0 if discovery else message_time)
            + tlv(4, b"" if discovery else self.credentials.user.encode())
            + tlv(4, b"" if discovery else bytes(12))
            + tlv(4, privacy),
        )
        header = tlv(
            48,
            integer(ident)
            + integer(1500)
            + tlv(4, bytes([4 if discovery else (7 if privacy else 5)]))
            + integer(3),
        )
        packet = tlv(48, integer(3) + header + tlv(4, security) + scoped)
        if not discovery:
            auth = parse(packet)["auth"]
            digest = hmac.digest(self.key, packet, self.hash_name)[:12]
            packet = packet[: auth[1]] + digest + packet[auth[2] :]
        return packet

    def verify(self, packet, m):
        auth = m["auth"]
        if (
            not m["flags"] & 1
            or auth[2] - auth[1] != 12
            or m["engine"] != self.engine
            or m["user"] != self.credentials.user.encode()
        ):
            raise ValueError("Unauthenticated v3 response")
        expected = packet[auth[1] : auth[2]]
        unsigned = packet[: auth[1]] + bytes(12) + packet[auth[2] :]
        if not hmac.compare_digest(
            expected, hmac.digest(self.key, unsigned, self.hash_name)[:12]
        ):
            raise ValueError("SNMP v3 authentication failed")

    def unwrap(self, packet, m):
        # Authentication must succeed before any ciphertext reaches a decryptor.
        self.verify(packet, m)
        if m["flags"] & 2:
            if self.credentials.privacy == "none":
                raise ValueError("Unexpected privacy")
            cipher = content(packet, m["scoped"])
            if not cipher:
                raise ValueError("Empty ciphertext")
            if self.credentials.privacy == "AES":
                iv = (
                    m["boots"].to_bytes(4, "big")
                    + m["time"].to_bytes(4, "big")
                    + m["privacy"]
                )
                plain = AES.new(
                    self.privacy_key[:16], AES.MODE_CFB, iv, segment_size=128
                ).decrypt(cipher)
                padding = 0
            else:
                if len(cipher) % 8:
                    raise ValueError("Invalid DES length")
                iv = bytes(a ^ b for a, b in zip(self.privacy_key[8:16], m["privacy"]))
                plain = DES.new(self.privacy_key[:8], DES.MODE_CBC, iv).decrypt(cipher)
                padding = 7
            scoped = node(plain, 0)
            if scoped[0] != 48 or len(plain) - scoped[2] > padding:
                raise ValueError("Invalid encrypted scope")
            m["pdu"] = scoped_pdu(plain, scoped, m["engine"])
            return plain
        if self.credentials.privacy != "none" and m["pdu"][0] != 0xA8:
            raise ValueError("Privacy downgrade rejected")
        return packet

    def exchange(self, udp, ident, request, name, next_oid=False, discovery=False):
        udp.send(self.packet(ident, request, name, next_oid, discovery))
        deadline = time.monotonic() + udp.gettimeout()
        while time.monotonic() < deadline:
            udp.settimeout(max(0.001, deadline - time.monotonic()))
            packet = udp.recv(1500)
            m = parse(packet)
            if m["id"] != ident:
                continue
            return packet, m
        raise TimeoutError("SNMP v3 timeout")

    def query(self, host, port, name, next_oid=False, timeout=4):
        request = secrets.randbelow(0x7FFFFFFE) + 1
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
            udp.settimeout(timeout)
            udp.connect((host, port))
            with self.lock:
                if not self.engine:
                    packet, m = self.exchange(
                        udp, request, request, name, discovery=True
                    )
                    if (
                        m["flags"] & 1
                        or content(packet, m["auth"])
                        or value(packet, m, request, True)[0]
                        != "1.3.6.1.6.3.15.1.1.4.0"
                    ):
                        raise ValueError("Invalid engine discovery report")
                    self.sync(m)
            for retry in range(2):
                udp.settimeout(timeout)
                packet, m = self.exchange(udp, request + retry, request, name, next_oid)
                packet = self.unwrap(packet, m)
                if m["pdu"][0] == 0xA8:
                    report = value(packet, m, request, True)[0]
                    if report == "1.3.6.1.6.3.15.1.1.2.0" and m["boots"] >= self.boots:
                        with self.lock:
                            self.sync(m)
                        continue
                    raise ValueError("SNMP v3 security report")
                if m["boots"] != self.boots or abs(m["time"] - self.clock()) > 150:
                    raise ValueError("SNMP v3 stale engine time")
                result = value(packet, m, request)
                if (not next_oid and result[0] != name) or (
                    next_oid and result[0] == name
                ):
                    raise ValueError("Unexpected OID")
                with self.lock:
                    if m["time"] >= self.clock():
                        self.sync(m)
                self.last_response = time.monotonic()
                self.failures = 0
                return result
            raise ValueError("SNMP v3 time synchronization failed")


_sessions = {}
_session_lock = threading.Lock()


def query(host, port, credentials, name, next_oid=False, timeout=4):
    key = (host, port, credentials)
    with _session_lock:
        if key not in _sessions:
            if len(_sessions) >= 8:
                _sessions.pop(next(iter(_sessions)))
            _sessions[key] = Session(credentials)
        session = _sessions[key]
    try:
        return session.query(host, port, name, next_oid, timeout)
    except OSError:
        with session.lock:
            session.failures += 1
            if session.failures >= 3 and time.monotonic() - session.last_response > 30:
                session.engine = b""
        raise
