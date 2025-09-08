// weather.cpp
#include "weather.h"
#include "config.h"
#include "secrets.h"
#include "storage.h"

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ------- Public globals (defined here, extern in weather.h) -------
String w_city = "";
String w_desc = "";
float  w_temp = NAN;
float  w_hum  = NAN;

bool   g_rainNext24h = false;

// ------- Local timers for "IfDue" wrappers -------
static uint32_t s_lastWeatherMs = 0;
static uint32_t s_lastRainMs    = 0;

// ---------------- Small helpers (no ArduinoJson) ----------------
static String findQuoted(const String& hay, const char* key, int from = 0) {
  int i = hay.indexOf(key, from);
  if (i < 0) return "";
  i += strlen(key);
  int e = hay.indexOf('\"', i);
  return (e > i) ? hay.substring(i, e) : "";
}
static String findNumber(const String& hay, const char* key, int from = 0) {
  int i = hay.indexOf(key, from);
  if (i < 0) return "";
  i += strlen(key);
  int e = i;
  while (e < (int)hay.length() && String("0123456789-+.eE").indexOf(hay[e]) >= 0) e++;
  return (e > i) ? hay.substring(i, e) : "";
}
static bool payloadSuggestsRain(String payload) {
  payload.toLowerCase();
  return payload.indexOf("\"rain\"") >= 0      ||
         payload.indexOf("\"main\":\"rain\"") >= 0 ||
         payload.indexOf(" drizzle") >= 0      ||
         payload.indexOf(" shower") >= 0       ||
         payload.indexOf(" rain") >= 0;
}

// ---------------- Public API ----------------
void weather_setup() {
  // Defaults so system starts permissive and UI shows "Allowed"
  g_rainNext24h = false;
  // Force first fetch to run immediately
  s_lastWeatherMs = millis() - OWM_REFRESH_MS - 1;
  s_lastRainMs    = millis() - RAIN_REFRESH_MS - 1;
}

void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Weather] WiFi not connected");
    return;
  }

  float lat = OWM_LAT, lon = OWM_LON; String citySaved=""; load_location(lat, lon, citySaved);
  String url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
               String(lat, 4) + "&lon=" + String(lon, 4) +
               "&appid=" + String(OWM_API_KEY) + "&units=metric";

  HTTPClient http;
  http.setTimeout(7000);

  for (int attempt = 1; attempt <= 3; ++attempt) {
    if (!http.begin(url)) {
      Serial.println("[Weather] http.begin() failed");
      delay(300);
      continue;
    }
    int code = http.GET();
    if (code != 200) {
      Serial.printf("[Weather] HTTP %d (attempt %d/3)\n", code, attempt);
      String body = http.getString();
      if (body.length()) Serial.printf("[Weather] Body: %s\n", body.c_str());
      http.end();
      delay(700);
      continue;
    }

    String payload = http.getString();
    http.end();

    String city = findQuoted(payload, "\"name\":\"");
    String desc = findQuoted(payload, "\"description\":\"");
    String temp = findNumber(payload, "\"temp\":");
    String hum  = findNumber(payload, "\"humidity\":");

    if (temp.length()) w_temp = temp.toFloat();
    if (hum.length())  w_hum  = hum.toFloat();
    if (city.length()) w_city = city;
    if (desc.length()) w_desc = desc;

    Serial.printf("[Weather] City=%s Temp=%.1fC Hum=%.0f%% Desc=%s\n",
                  w_city.c_str(), w_temp, w_hum, w_desc.c_str());
    return;
  }

  Serial.println("[Weather] Failed after retries");
}

void fetchRainForecast() {
  // Default to NO rain unless proven otherwise (prevents false block)
  g_rainNext24h = false;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[RainForecast] WiFi not connected -> assume NO rain");
    return;
  }

  float lat = OWM_LAT, lon = OWM_LON; String citySaved=""; load_location(lat, lon, citySaved);
  // Only next 8 x 3h buckets (~24h)
  String url = "http://api.openweathermap.org/data/2.5/forecast?lat=" +
               String(lat, 4) + "&lon=" + String(lon, 4) +
               "&appid=" + String(OWM_API_KEY) +
               "&units=metric&cnt=8";

  HTTPClient http;
  http.setTimeout(7000);

  for (int attempt = 1; attempt <= 3; ++attempt) {
    if (!http.begin(url)) {
      Serial.println("[RainForecast] http.begin() failed");
      delay(300);
      continue;
    }
    int code = http.GET();
    if (code != 200) {
      Serial.printf("[RainForecast] HTTP %d (attempt %d/3)\n", code, attempt);
      String body = http.getString();
      if (body.length()) Serial.printf("[RainForecast] Body: %s\n", body.c_str());
      http.end();
      delay(700);
      continue;
    }

    String payload = http.getString();
    http.end();

    g_rainNext24h = payloadSuggestsRain(payload);
    Serial.printf("[RainForecast] Next 24h rain: %s\n", g_rainNext24h ? "YES" : "NO");
    return;
  }

  Serial.println("[RainForecast] Failed after retries -> assume NO rain");
  // g_rainNext24h remains false
}

void fetchWeatherIfDue() {
  uint32_t now = millis();
  if ((uint32_t)(now - s_lastWeatherMs) >= OWM_REFRESH_MS) {
    fetchWeather();
    s_lastWeatherMs = millis();
  }
}
void fetchRainForecastIfDue() {
  uint32_t now = millis();
  if ((uint32_t)(now - s_lastRainMs) >= RAIN_REFRESH_MS) {
    fetchRainForecast();
    s_lastRainMs = millis();
  }
}

// Force wrappers (exported via weather.h)
void forceFetchWeather() {
  fetchWeather();
  s_lastWeatherMs = millis();
}
void forceFetchRain() {
  fetchRainForecast();
  s_lastRainMs = millis();
}