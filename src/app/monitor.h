#pragma once
#include "telemetry.h"

void weatherMonitor() {
  const uint32_t ttl = max(1800000u, cfg.weatherInterval * 180000u);
  if (weather.valid && millis() - weather.fetched > ttl) weather.valid = false;
  if (!cfg.weatherEnabled || ap || WiFi.status() != WL_CONNECTED)
    return;
  uint32_t now = millis();
  int result = weatherClient.poll(weather);
  if (result)
    Serial.printf("WEATHER result=%d city=%s temp=%.1f humidity=%.0f error=%s heap=%u\n", result,
                  weather.cityEnglish.c_str(), weather.temperature, weather.humidity,
                  weather.error.c_str(), ESP.getFreeHeap());
  uint32_t interval = weather.error.length() ? Sampling::WeatherRetry : cfg.weatherInterval * 60000;
  if (!weatherClient.busy() && (!weatherStarted || now - weatherAttempt >= interval)) {
    weatherStarted = true;
    weatherAttempt = now;
    if (!weatherClient.query(cfg.cityCode))
      weather.error = "Weather connection failed";
  }
}
