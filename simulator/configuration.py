"""Configuration disk I/O; telemetry caches are deliberately not persisted."""

import json
import os
import copy
import re
import ipaddress


from credentials import seal, unseal, password_hash, default_password
import secrets

def config_key(path, create=False):
    target = path.with_suffix(".key")
    if target.exists():
        key = target.read_bytes()
        if len(key) != 64: raise ValueError("Invalid configuration key")
        return key
    if not create: raise ValueError("Missing configuration key")
    key = secrets.token_bytes(64)
    fd = os.open(target, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "wb") as file:
        file.write(key); file.flush(); os.fsync(file.fileno())
    return key

def write_atomic(path, config):
    temporary = path.with_suffix(".tmp")
    saved = copy.deepcopy(config)
    saved["schema"] = 3
    saved["adminDefault"] = default_password(saved)
    if not saved["admin"].startswith("p2$"): saved["admin"] = password_hash(saved["admin"])
    try:
        plain = json.dumps(saved, ensure_ascii=False, separators=(",", ":")).encode()
        if len(plain) > 8192: raise ValueError("Configuration too large")
        with temporary.open("wb") as file:
            file.write(seal(plain, config_key(path, create=True)))
            file.flush(); os.fsync(file.fileno())
        temporary.replace(path)
        config["admin"] = saved["admin"]
        config["adminDefault"] = saved["adminDefault"]
    except OSError:
        temporary.unlink(missing_ok=True)
        raise

def read_config(path):
    if path.stat().st_size > 8244: raise ValueError("Configuration too large")
    saved = json.loads(unseal(path.read_bytes(), config_key(path)))
    if saved.get("schema") != 3 or not saved.get("admin", "").startswith("p2$"):
        raise ValueError("Unsupported configuration schema")
    if not isinstance(saved, dict):
        raise ValueError("Configuration must be an object")
    machines = saved.get("machines", [])
    if (
        not isinstance(machines, list)
        or len(machines) > 6
        or any(not isinstance(m, dict) for m in machines)
    ):
        raise ValueError("Invalid device list")
    active = saved.get("activeMachine", 0)
    if not isinstance(active, int) or active < 0 or active >= max(1, len(machines)):
        raise ValueError("Invalid active device")
    return saved


def normalize_config(saved, defaults, machine_fields):
    """Validate inactive profiles as carefully as the selected device."""
    result = copy.deepcopy(defaults)
    for key, default in defaults.items():
        value = saved.get(key)
        if type(value) is type(default):
            result[key] = copy.deepcopy(value)
    limits = {
        "port": (1, 65535),
        "interval": (3, 60),
        "brightness": (5, 100),
        "weatherInterval": (10, 120),
        "rotation": (0, 3),
        "snooze": (1, 30),
        "albumInterval": (3, 300),
        "alarmDays": (1, 127),
        "alarmTune": (0, 3),
    }
    choices = {
        "snmpVersion": ("2c", "3"),
        "snmpAuth": ("MD5", "SHA"),
        "snmpPrivacy": ("none", "DES", "AES"),
        "language": ("zh", "en"),
        "adminLanguage": ("zh", "en"),
        "mode": ("snmp", "demo"),
    }
    lengths = {
        "snmpUser": 32,
        "snmpPassword": 64,
        "snmpPrivacyPassword": 64,
        "community": 64,
        "admin": 100,
        "ssid": 32,
        "pass": 63,
        "ntpServer": 63,
        "machineName": 48,
    }

    def normalize(fields):
        for key, (low, high) in limits.items():
            if key in fields:
                value = fields[key]
                fields[key] = (
                    min(high, max(low, value)) if type(value) is int else defaults[key]
                )
        for key, values in choices.items():
            if key in fields and fields[key] not in values:
                fields[key] = defaults[key]
        for key, length in lengths.items():
            if key in fields:
                text = (
                    fields[key]
                    if isinstance(fields[key], str)
                    else defaults.get(key, "")
                )
                text = "".join(c for c in text if ord(c) >= 32 and ord(c) != 127)
                fields[key] = text.encode()[:length].decode("utf8", errors="ignore")
        for key in ("clockColor", "minuteColor", "nasBackground", "cardBackground"):
            if key in fields and not re.fullmatch(r"#[0-9a-fA-F]{6}", str(fields[key])):
                fields[key] = defaults[key]
        for key in ("iface", "storage"):
            if key in fields and (
                not isinstance(fields[key], str)
                or (fields[key] and not re.fullmatch(r"[0-9]{1,10}", fields[key]))
            ):
                fields[key] = ""
        if "nas" in fields:
            try:
                ipaddress.IPv4Address(fields["nas"])
            except (ValueError, TypeError):
                fields["nas"] = defaults["nas"]

    normalize(result)
    for i, machine in enumerate(result["machines"]):
        profile = {k: machine.get(k, defaults[k]) for k in machine_fields}
        normalize(profile)
        name = machine.get("name", "")
        profile["name"] = str(name).encode()[:48].decode("utf8", errors="ignore")
        result["machines"][i] = profile
    if result["machines"]:
        active = result["machines"][result["activeMachine"]]
        result.update({k: active[k] for k in machine_fields})
        result["machineName"] = active["name"]
    if not re.fullmatch(r"p2\$[0-9a-f]{32}\$[0-9a-f]{64}", result["admin"]):
        result["admin"] = defaults["admin"]
        result["adminDefault"] = True
    if result["home"] not in (0, 1, 2, 4, 5, 6, 7, 8, 9):
        result["home"] = defaults["home"]
    return result
