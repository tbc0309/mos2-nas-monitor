#pragma once
// A bounded asynchronous probe: HTTP handlers never wait for a weather body.
namespace WeatherValidation {
Weather::Client client;
String code, result = "idle";
uint32_t completed = 0;
bool syntax(const String &value) {
  if (value.length() != 9) return false;
  for (char c : value) if (c < '0' || c > '9') return false;
  return value[0] != '0';
}
bool ready(const String &value) {
  return WeatherLocations::listed(value) || value == cfg.weatherCityCode ||
         (code == value && result == "valid" && millis()-completed < 600000);
}
void tick() {
  if (!client.busy()) return;
  Weather::Reading reading;
  int state = client.poll(reading);
  if (!state) return;
  result = state > 0 ? "valid" : reading.error == "Forecast only" ? "forecast" :
           (reading.error == "Invalid weather data" || reading.error == "Weather city not found") ? "invalid" : "unavailable";
  completed = millis();
}
void check() {
  if (!authorized()) return;
  if (web.arg("token") != csrf) { web.send(403,"text/plain","Reload page"); return; }
  String value = web.arg("code");
  if (!syntax(value)) { web.send(400,"application/json","{\"status\":\"invalid\"}"); return; }
  if (client.busy() && code != value) {
    web.send(409,"application/json","{\"status\":\"busy\"}"); return;
  }
  if (ready(value)) { web.send(200,"application/json","{\"status\":\"valid\"}"); return; }
  if (code != value || (!client.busy() && millis()-completed >= 3000)) {
    client.cancel(); weatherClient.cancel(); code = value; result = "pending";
    if (!client.query(code)) { result = "unavailable"; completed = millis(); }
  }
  web.send(200,"application/json", "{\"status\":\""+result+"\"}");
}
} // namespace WeatherValidation
