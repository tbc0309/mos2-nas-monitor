"""Bounded local HTTP access policy; no credentials are logged here."""

import time
from urllib.parse import urlsplit


class LoginGuard:
    def __init__(self):
        self.failures = 0
        self.until = 0.0

    def blocked(self):
        return time.monotonic() < self.until

    def failed(self):
        self.failures = min(10, self.failures + 1)
        if self.failures >= 5:
            self.until = time.monotonic() + min(300, 30 * 2 ** (self.failures - 5))

    def clear(self):
        self.failures = 0
        self.until = 0.0


def local_request(headers, port):
    """Reject DNS rebinding and cross-origin browser writes to the local service."""
    host = headers.get("Host", "")
    allowed = {f"127.0.0.1:{port}", f"localhost:{port}", f"[::1]:{port}"}
    if host not in allowed:
        return False
    origin = headers.get("Origin")
    if origin:
        try:
            parsed = urlsplit(origin)
            if parsed.scheme != "http" or parsed.netloc != host:
                return False
        except ValueError:
            return False
    return headers.get("Sec-Fetch-Site") != "cross-site"
