#pragma once
// Serial-only diagnostics never include form bodies, cookies or credentials.
// The main loop owns measurements; no timer/interrupt performs logging or I/O.
namespace RuntimeHealth {
uint32_t minimumHeap = UINT32_MAX, minimumBlock = UINT32_MAX;
uint32_t requestStarted = 0, requests = 0, previousReport = 0;
bool requestPending = false;
struct Checkpoint { uint32_t magic, phase, at, heap; };
inline void checkpoint(uint32_t phase) {
  Checkpoint value = {0x4d4f5352u, phase, millis(), ESP.getFreeHeap()};
  ESP.rtcUserMemoryWrite(96, reinterpret_cast<uint32_t *>(&value), sizeof value);
}
inline void previousCheckpoint() {
  Checkpoint value = {};
  if (ESP.rtcUserMemoryRead(96, reinterpret_cast<uint32_t *>(&value), sizeof value) && value.magic == 0x4d4f5352u)
    Serial.printf("PREVIOUS loopPhase=%u at=%u heap=%u\n", value.phase, value.at, value.heap);
}
inline void sample() {
  minimumHeap = min(minimumHeap, ESP.getFreeHeap());
  minimumBlock = min(minimumBlock, ESP.getMaxFreeBlockSize());
}
inline void begin(const String &method, const String &url) {
  sample(); requestStarted = millis(); requestPending = true; ++requests;
  String path = url.substring(0, url.indexOf('?') < 0 ? url.length() : url.indexOf('?'));
  Serial.printf("HTTP begin n=%u method=%s path=%s heap=%u block=%u stack=%u\n",
    requests, method.c_str(), path.c_str(), ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(), ESP.getFreeContStack());
}
inline void finish() {
  if (!requestPending) return;
  requestPending = false; sample();
  Serial.printf("HTTP end n=%u ms=%u heap=%u block=%u stack=%u\n", requests,
    uint32_t(millis()-requestStarted), ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(), ESP.getFreeContStack());
}
inline void report(unsigned page, unsigned machine) {
  sample();
  if (millis()-previousReport < 10000) return;
  previousReport = millis();
  Serial.printf("RUNTIME up=%u page=%u machine=%u heap=%u block=%u minHeap=%u minBlock=%u frag=%u stack=%u requests=%u\n",
    unsigned(millis()),page,machine,ESP.getFreeHeap(),ESP.getMaxFreeBlockSize(),minimumHeap,minimumBlock,
    ESP.getHeapFragmentation(),ESP.getFreeContStack(),requests);
}
}
