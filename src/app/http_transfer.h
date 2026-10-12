#pragma once
#include "runtime_workspace.h"
void sendFlashContent(const char *source, size_t length) {
  while (length && web.client().connected()) {
    const size_t count = min(length, sizeof(RuntimeWorkspace::bytes));
    memcpy_P(RuntimeWorkspace::bytes, source, count);
    // RAM supports direct Stream peek; Flash's core fallback uses 64-byte
    // stack copies, causing excessive ACK waits with synchronous TCP writes.
    web.sendContent(reinterpret_cast<const char *>(RuntimeWorkspace::bytes), count);
    source += count; length -= count;
    yield();
  }
}
void sendFlash(int code, const char *type, const char *source, size_t length) {
  web.setContentLength(length);
  web.send(code, type, "");
  sendFlashContent(source, length);
}
void sendFlash(int code, const char *type, const char *source) {
  sendFlash(code, type, source, strlen_P(source));
}
