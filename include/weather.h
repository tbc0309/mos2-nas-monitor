#pragma once
#include "sampling_policy.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#ifndef WEATHER_HOST_TEST
#include <ESP8266WiFi.h>
#endif
#include <math.h>
namespace Weather {
struct Reading {
  String city, cityEnglish, condition, conditionEnglish, wind, windEnglish, observed,
      error = "Waiting for weather";
  float temperature = -999, humidity = -1;
  int aqi = -1, code = -1;
  uint32_t fetched = 0;
  bool valid = false;
};
inline bool numeric(const String &s, float low, float high, float &out) {
  if (!s.length())
    return false;
  char *end = nullptr;
  float n = strtof(s.c_str(), &end);
  if (end == s.c_str() || *end || !isfinite(n) || n < low || n > high)
    return false;
  out = n;
  return true;
}
inline bool parse(const String &body, const String &cityCode, Reading &out) {
  int begin = body.indexOf("var dataSK");
  if (begin < 0)
    return false;
  begin = body.indexOf('{', begin);
  int end = body.indexOf(";var dataZS", begin);
  if (end < 0 || end - begin > 2048 || begin < 0)
    return false;
  StaticJsonDocument<1536> doc;
  if (deserializeJson(doc, body.substring(begin, end).c_str()))
    return false;
  if (String(doc["city"] | "") != cityCode)
    return false;
  float t, h;
  String humidity = doc["SD"] | "";
  humidity.replace("%", "");
  if (!numeric(doc["temp"] | "", -80, 65, t) || !numeric(humidity, 0, 100, h))
    return false;
  Reading next;
  next.city = doc["cityname"] | "";
  next.cityEnglish = doc["nameen"] | "";
  next.condition = doc["weather"] | "";
  next.conditionEnglish = doc["weathere"] | "";
  next.wind = String(doc["WD"] | "") + " " + String(doc["WS"] | "");
  next.windEnglish = String(doc["wde"] | "") + " " + String(doc["wse"] | "");
  next.observed = doc["time"] | "";
  next.temperature = t;
  next.humidity = h;
  float a;
  if (numeric(doc["aqi"] | "", 0, 500, a))
    next.aqi = int(a);
  String code = doc["weathercode"] | "";
  if (code.length() > 1 && numeric(code.substring(1), 0, 99, a))
    next.code = int(a);
  if (!next.city.length() || !next.condition.length())
    return false;
  next.valid = true;
  next.error = "";
  next.fetched = millis();
  out = next;
  return true;
}
#ifndef WEATHER_HOST_TEST
class Client {
  WiFiClient socket;
  String response, requestedCity;
  uint32_t started = 0;
  bool active = false;
  int finish(Reading &r, bool ok, const String &error) {
    socket.stop();
    active = false;
    response = String();
    if (!ok)
      r.error = error;
    return ok ? 1 : -1;
  }

public:
  bool busy() const {
    return active;
  }
  void cancel() {
    socket.stop();
    active = false;
    response = String();
  }
  bool query(const String &city) {
    IPAddress host;
    requestedCity = city;
    if (WiFi.hostByName("d1.weather.com.cn", host, 250) != 1)
      return false;
    socket.setTimeout(250);
    if (!socket.connect(host, 80))
      return false;
    response = "";
    response.reserve(6144);
    started = millis();
    active = true;
    socket.print("GET /weather_index/" + city + ".html?_=" + String(millis()) +
                 " HTTP/1.0\r\nHost: d1.weather.com.cn\r\nUser-Agent: Mozilla/5.0\r\nReferer: "
                 "http://www.weather.com.cn/\r\nConnection: close\r\n\r\n");
    return true;
  }
  int poll(Reading &r) {
    if (!active)
      return 0;
    unsigned budget = 768;
    while (socket.available() && budget--) {
      if (response.length() >= 8192)
        return finish(r, false, "Response too large");
      response += char(socket.read());
    }
    if (millis() - started > Sampling::WeatherTimeout)
      return finish(r, false, "Weather request timed out");
    if (socket.connected() || socket.available())
      return 0;
    int split = response.indexOf("\r\n\r\n");
    if (split < 0 || !response.startsWith("HTTP/1.") || response.substring(9, 12) != "200")
      return finish(r, false, "Weather HTTP error");
    String headers = response.substring(0, split);
    headers.toLowerCase();
    if (headers.indexOf("transfer-encoding: chunked") >= 0)
      return finish(r, false, "Unsupported HTTP encoding");
    bool ok = parse(response.substring(split + 4), requestedCity, r);
    return finish(r, ok, "Invalid weather data");
  }
};
#endif
} // namespace Weather
