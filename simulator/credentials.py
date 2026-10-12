"""Password verifiers and authenticated local configuration envelopes."""
import hashlib, hmac, secrets
from Crypto.Cipher import AES
MAGIC = b"MOSCFG3\0"
ITERATIONS = 4000

def password_hash(password):
    salt = secrets.token_bytes(16)
    value = hashlib.pbkdf2_hmac("sha256", password.encode(), salt, ITERATIONS)
    return f"p2${salt.hex()}${value.hex()}"

def password_valid(password, verifier):
    if not verifier.startswith("p2$"):
        return hmac.compare_digest(password, verifier)  # Unsaved in-memory defaults only.
    try:
        _, salt, expected = verifier.split("$")
        if len(salt) != 32 or len(expected) != 64: return False
        actual = hashlib.pbkdf2_hmac("sha256", password.encode(), bytes.fromhex(salt), ITERATIONS)
        return hmac.compare_digest(actual, bytes.fromhex(expected))
    except ValueError:
        return False

def default_password(config):
    value = config.get("admin", "12345678")
    return config.get("adminDefault", False) if value.startswith("p2$") else value == "12345678"

def seal(plain, key, nonce=None):
    if len(key) != 64: raise ValueError("Invalid key")
    nonce = nonce or secrets.token_bytes(12)
    if len(nonce) != 12: raise ValueError("Invalid nonce")
    body = MAGIC + nonce + AES.new(key[:32], AES.MODE_CTR, nonce=nonce, initial_value=0).encrypt(plain)
    return body + hmac.digest(key[32:], body, "sha256")

def unseal(data, key):
    if len(key) != 64 or not 52 <= len(data) <= 8244 or data[:8] != MAGIC:
        raise ValueError("Invalid encrypted configuration")
    if not hmac.compare_digest(hmac.digest(key[32:], data[:-32], "sha256"), data[-32:]):
        raise ValueError("Configuration authentication failed")
    return AES.new(key[:32], AES.MODE_CTR, nonce=data[8:20], initial_value=0).decrypt(data[20:-32])
