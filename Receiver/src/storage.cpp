// storage.cpp
#include "storage.h"
#include "config.h"

Preferences g_prefs;

static String joinCSV(const String arr[], size_t n, char sep) {
  String s;
  for (size_t i = 0; i < n; ++i) { s += arr[i]; if (i + 1 < n) s += sep; }
  return s;
}

void storage_begin() {
  // Ensure NVS namespaces exist to avoid noisy NOT_FOUND logs on first boot
  const char* namespaces[] = {"names", "pumpctrl", "timers", "location", "settings"};
  for (size_t i = 0; i < sizeof(namespaces)/sizeof(namespaces[0]); ++i) {
    if (!g_prefs.begin(namespaces[i], true)) {
      g_prefs.end();
      g_prefs.begin(namespaces[i], false); // create namespace
      g_prefs.end();
    } else {
      g_prefs.end();
    }
  }
}

void csv_to_names(const String& s, String out[10], char sep) {
  int start = 0, idx = 0;
  while (idx < 10) {
    int pos = s.indexOf(sep, start);
    if (pos < 0) pos = s.length();
    out[idx++] = s.substring(start, pos);
    start = pos + 1;
    if (pos >= (int)s.length()) break;
  }
  for (; idx < 10; ++idx) out[idx] = String("Sensor ") + (idx+1);
}

String names_csv(const String names[10]) { return joinCSV(names, 10, '|'); }

void load_names(String names[10]) {
  g_prefs.begin("names", true);
  String saved = g_prefs.getString("labels", "");
  g_prefs.end();
  if (saved.length() == 0) {
    for (int i=0;i<10;i++) names[i] = String("Sensor ") + (i+1);
  } else {
    csv_to_names(saved, names);
  }
}

void save_names(const String names[10]) {
  g_prefs.begin("names", false);
  g_prefs.putString("labels", names_csv(names));
  g_prefs.end();
}

void load_pumps(PumpPersist& p) {
  g_prefs.begin("pumpctrl", true);
  Serial.println("[Storage] Loading pump configuration...");
  for (int i=0;i<4;i++) {
    p.enabled[i] = g_prefs.getBool(("pump"+String(i)).c_str(), true);
    p.manual[i]  = g_prefs.getBool(("man"+String(i)).c_str(),  false);
    p.overridePct[i] = g_prefs.getUChar(("ovr"+String(i)).c_str(), 0);
    // Load cutoff with a more sensible default - if not set, use DEFAULT_CUTOFF_PCT
    p.cutoffPct[i]   = g_prefs.getUChar(("cut"+String(i)).c_str(), DEFAULT_CUTOFF_PCT);
    Serial.printf("[Storage] Pump %d: enabled=%d, manual=%d, override=%d, cutoff=%d\n", 
                  i, p.enabled[i], p.manual[i], p.overridePct[i], p.cutoffPct[i]);
  }
  g_prefs.end();
}

void save_pump_enabled(int idx, bool st) {
  g_prefs.begin("pumpctrl", false);
  g_prefs.putBool(("pump"+String(idx)).c_str(), st);
  g_prefs.end();
}

void save_pump_override(int idx, bool manual, uint8_t pct) {
  g_prefs.begin("pumpctrl", false);
  g_prefs.putBool(("man"+String(idx)).c_str(), manual);
  g_prefs.putUChar(("ovr"+String(idx)).c_str(), pct);
  g_prefs.end();
}

void save_cutoff_pct(int idx, uint8_t pct) {
  g_prefs.begin("pumpctrl", false);
  g_prefs.putUChar(("cut"+String(idx)).c_str(), pct);
  g_prefs.end();
}

void load_durations(uint32_t& lockoutMs, uint32_t& manualMs) {
  g_prefs.begin("timers", true);
  lockoutMs = g_prefs.getULong("lockout_ms", 0);
  manualMs  = g_prefs.getULong("manual_ms",  0);
  g_prefs.end();
}

void save_durations(uint32_t lockoutMs, uint32_t manualMs) {
  g_prefs.begin("timers", false);
  g_prefs.putULong("lockout_ms", lockoutMs);
  g_prefs.putULong("manual_ms",  manualMs);
  g_prefs.end();
}

void load_location(float& lat, float& lon, String& city) {
  g_prefs.begin("location", true);
  lat = g_prefs.getFloat("lat", OWM_LAT);
  lon = g_prefs.getFloat("lon", OWM_LON);
  city = g_prefs.getString("city", "");
  g_prefs.end();
}

void save_location(float lat, float lon, const String& city) {
  g_prefs.begin("location", false);
  g_prefs.putFloat("lat", lat);
  g_prefs.putFloat("lon", lon);
  g_prefs.putString("city", city);
  g_prefs.end();
}