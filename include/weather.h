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
inline String englishCondition(int code) {
  switch(code) {
    case 0: return "Sunny"; case 1: return "Cloudy"; case 2: return "Overcast";
    case 3: return "Showers"; case 4: return "Thunderstorms"; case 5: return "Thunderstorms with hail";
    case 6: return "Sleet"; case 7: return "Light rain"; case 8: return "Rain";
    case 9: return "Heavy rain"; case 10: case 11: case 12: return "Rainstorm";
    case 13: return "Snow showers"; case 14: return "Light snow";
    case 15: return "Snow"; case 16: return "Heavy snow"; case 17: return "Blizzard";
    case 18: return "Fog"; case 19: return "Freezing rain"; case 20: case 31: return "Sandstorm";
    case 21: case 22: case 23: case 24: case 25: case 301: return "Rain";
    case 26: case 27: case 28: case 302: return "Snow";
    case 29: return "Dust"; case 30: return "Blowing sand";
    case 32: case 49: return "Dense fog"; case 53: case 54: case 55: case 56: return "Haze";
    default: return "Weather";
  }
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
  float t, h = -1;
  String humidity = doc["SD"] | "";
  humidity.replace("%", "");
  if (!numeric(doc["temp"] | "", -80, 65, t))
    return false;
  if (!numeric(humidity, 0, 100, h)) h = -1;
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
  if (code.length() > 1 && numeric(code.substring(1), 0, 999, a))
    next.code = int(a);
  if (!next.conditionEnglish.length()) next.conditionEnglish = englishCondition(next.code);
  if (!next.city.length() || !next.condition.length())
    return false;
  next.valid = true;
  next.error = "";
  next.fetched = millis();
  out = next;
  return true;
}
// Extract only the bounded dataSK object; discard forecast/advice sections.
class Extractor {
  String marker;
  int depth = 0;
  bool selected = false, quoted = false, escaped = false;
public:
  String object;
  bool complete = false, invalid = false, forecast = false;
  void feed(char c) {
    if (complete || invalid) return;
    if (!selected) {
      marker += c;
      if (marker.indexOf("var fc=") >= 0 || marker.indexOf("var fc =") >= 0) { forecast = true; return; }
      if (marker.indexOf("var dataSK") >= 0) { selected = true; marker = String(); }
      else if (marker.length() > 24) marker = marker.substring(1);
      return;
    }
    if (!depth && c != '{') return;
    if (object.length() >= 2048) { invalid = true; return; }
    object += c;
    if (escaped) { escaped = false; return; }
    if (quoted && c == '\\') { escaped = true; return; }
    if (c == '"') { quoted = !quoted; return; }
    if (!quoted) {
      if (c == '{') ++depth;
      else if (c == '}' && --depth == 0) complete = true;
    }
  }
};
#ifndef WEATHER_HOST_TEST
class Client {
  WiFiClient socket;
  String response, requestedCity;
  uint32_t started = 0;
  bool active = false, headersDone = false;
  Extractor extractor;
  int finish(Reading &r, bool ok, const String &error) {
    socket.stop();
    active = false;
    response = String();
    extractor = Extractor();
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
    // A partial dataSK object can be 2 KiB. Cancellation must release it too.
    extractor = Extractor();
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
    response.reserve(512);
    headersDone = false;
    extractor = Extractor();
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
      char c = char(socket.read());
      if (!headersDone) {
        if (response.length() >= 1536) return finish(r, false, "Response too large");
        response += c;
        if (response.endsWith("\r\n\r\n")) {
          if (response.startsWith("HTTP/1.") && response.substring(9, 12) == "404")
            return finish(r, false, "Weather city not found");
          if (!response.startsWith("HTTP/1.") || response.substring(9, 12) != "200")
            return finish(r, false, "Weather HTTP error");
          response.toLowerCase();
          if (response.indexOf("transfer-encoding: chunked") >= 0)
            return finish(r, false, "Unsupported HTTP encoding");
          headersDone = true; response = String();
        }
      } else {
        extractor.feed(c);
        if (extractor.forecast) return finish(r, false, "Forecast only");
        if (extractor.invalid) return finish(r, false, "Response too large");
        if (extractor.complete) {
          bool ok = parse("var dataSK=" + extractor.object + ";var dataZS", requestedCity, r);
          return finish(r, ok, "Invalid weather data");
        }
      }
    }
    if (millis() - started > Sampling::WeatherTimeout)
      return finish(r, false, "Weather request timed out");
    if (socket.connected() || socket.available())
      return 0;
    return finish(r, false, "Invalid weather data");
  }
};
#endif
} // namespace Weather
