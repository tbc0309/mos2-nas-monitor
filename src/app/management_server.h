#pragma once
#include <ESP8266WebServer.h>

// ESP8266WebServer retains decoded arguments until the next request. A full
// settings form can otherwise occupy the memory needed by the LCD strip buffer.
// Release only after handleClient returns: route callbacks have finished then.
class ManagementServer : public ESP8266WebServer {
  bool releasePending = false;
public:
  explicit ManagementServer(int port) : ESP8266WebServer(port) {}
  void releaseArgumentsWhenHandled() { releasePending = true; }
  void releaseHandledArguments() {
    if (!releasePending) return;
    releasePending = false;
    delete[] _currentArgs;
    _currentArgs = nullptr;
    _currentArgCount = _currentArgsHavePlain = 0;
    // Header names remain registered; values belong only to the completed request.
    for (int i = 0; i < _headerKeysCount; ++i) _currentHeaders[i].value = String();
  }
};

