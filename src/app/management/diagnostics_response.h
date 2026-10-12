#pragma once
// Capture memory before response serialization so diagnostics do not measure
// their own temporary JSON allocations. The shared sink has a fixed 1 KiB cap.
void managementDiagnostics() {
  if (!authorized()) return;
  const uint32_t heap = ESP.getFreeHeap(), block = ESP.getMaxFreeBlockSize();
  const unsigned fragmentation = ESP.getHeapFragmentation(), stack = ESP.getFreeContStack();
  const uint32_t now = millis();
  auto emit = [&](Print &out) {
    JsonFields root(out);
    root.field("snmpPaused", !snmpCollectionEnabled);
    root.field("weatherPaused", !weatherCollectionEnabled);
    root.field("cpuMHz", ESP.getCpuFreqMHz());
    root.field("deviceUptimeMs", now);
    root.field("touchEdges", TouchInput::edges);
    root.field("touchGestures", TouchInput::gestures);
    root.field("touchQueueDrops", TouchInput::actions.dropped);
    root.field("touchEdgeOverflows", TouchInput::overflows);
    root.field("touchSwitches", touchSwitches);
    root.field("touchBlocked", touchBlocked);
    root.field("selectionSaveFailures", selectionSaveFailures);
    root.field("selectionPending", machineSelectionDirty);
    root.field("credentialDeriveMicros", Credentials::lastDeriveMicros);
    root.field("renderMicros", renderMicros);
    root.field("renderRows", renderRows);
    root.field("partialDraws", partialDraws);
    root.field("renderMaxMicros", renderMaxMicros);
    root.field("bufferedDraws", bufferedDraws);
    root.field("displayBufferFailures", displayBufferFailures);
    root.field("storageRequests", storageCollector.requestsSent());
    root.field("storageDiscoveryActive", storageCollector.discoveryActive());
    root.field("interfaceRequests", networkInterfaces.requestsSent());
    root.field("animationFrames", animationDraws);
    root.field("phase", phase);
    root.field("discover", discover);
    root.field("heap", heap);
    root.field("maxFreeBlock", block);
    root.field("heapFragmentation", fragmentation);
    root.field("freeStack", stack);
    root.field("minimumHeap", RuntimeHealth::minimumHeap);
    root.field("minimumBlock", RuntimeHealth::minimumBlock);
    root.field("fontReady", FontResource::available());
    root.field("fontReads", FontResource::reads);
    root.field("fontCacheHits", FontResource::cacheHits);
    root.field("fontReadErrors", FontResource::readErrors);
    root.field("fontCheckedMask", FontResource::checkedBanks);
    root.field("fontFailedMask", FontResource::failedBanks);
    root.field("fontBankOpens", FontResource::bankOpens);
    root.field("fontCacheBytes", FontResource::cacheBytes());
    root.field("gateway", WiFi.gatewayIP().toString());
    Telemetry::emitDiagnostics(root, out, now);
    root.finish();
  };
  ConfigCountSink count; emit(count);
  // Known length avoids small chunk-header writes and their TCP ACK waits.
  web.setContentLength(count.count); web.send(200, "application/json", "");
  ConfigWebSink out; emit(out); out.flush();
}
