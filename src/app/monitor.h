#pragma once
#include "telemetry.h"
String automaticDeviceName() {
  return IdentityLabel::automatic(Telemetry::systemHostName,metric.model,Telemetry::verifiedSynologyModel,Telemetry::deviceProfile);
}

void weatherMonitor() {
  if (WeatherValidation::client.busy()) return;
  if (!activity.weather(millis())) { weatherClient.cancel(); return; }
  const uint32_t ttl = max(1800000u, cfg.weatherRefreshMinutes * 180000u);
  if (weather.valid && millis() - weather.fetched > ttl) weather.valid = false;
  if (!cfg.weatherEnabled || ap || WiFi.status() != WL_CONNECTED)
    return;
  uint32_t now = millis();
  int result = weatherClient.poll(weather);
  if (result) weatherFailures = result > 0 ? 0 : min(weatherFailures + 1, 5u);
  if (result)
    Serial.printf("WEATHER result=%d city=%s temp=%.1f humidity=%.0f error=%s heap=%u\n", result,
                  weather.cityEnglish.c_str(), weather.temperature, weather.humidity,
                  weather.error.c_str(), ESP.getFreeHeap());
  uint32_t interval = weather.error.length() ? Sampling::weatherRetry(weatherFailures) : cfg.weatherRefreshMinutes * 60000;
  if (!weatherClient.busy() && (!weatherStarted || now - weatherAttempt >= interval)) {
    weatherStarted = true;
    weatherAttempt = now;
    if (!weatherClient.query(cfg.weatherCityCode)) {
      weather.error = "Weather connection failed";
      weatherFailures = min(weatherFailures + 1, 5u);
    }
  }
}
