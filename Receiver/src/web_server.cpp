// web_server.cpp
#include "web_server.h"
#include "config.h"
#include "web_auth.h"
#include "index_html2.h"
#include "storage.h"
#include "weather.h"
#include "pumps.h"

WebServer server(WEB_PORT);
String g_sensorNames[10];

// --- from main.cpp ---
extern float maxPumpTemp;          // current max temp setting
extern void handleSetMaxTemp();    // GET /setMaxTemp?value=XX.X

static String jsonData() {
  String j = "{\"soil\":[";
  for (int i = 0; i < 10; ++i) { j += String(g_soil[i]); if (i < 9) j += ","; }
  j += "],\"t\":"; j += isnan(g_tAir) ? "null" : String(g_tAir,1);
  j += ",\"h\":"; j += isnan(g_hAir) ? "null" : String(g_hAir,0);
  j += ",\"v\":"; j += isnan(g_vBus) ? "null" : String(g_vBus,2);
  j += ",\"i\":"; j += isnan(g_i_mA) ? "null" : String(g_i_mA,1);
  j += ",\"p\":"; j += isnan(g_p_mW) ? "null" : String(g_p_mW,1);
  j += ",\"weather\":{";
  j += "\"city\":\"" + w_city + "\",";
  j += "\"desc\":\"" + w_desc + "\",";
  j += "\"t\":"; j += isnan(w_temp) ? "null" : String(w_temp,1);
  j += ",\"h\":"; j += isnan(w_hum) ? "null" : String(w_hum,0);
  // provide coordinates for UI map using configured/current location
  float clat = OWM_LAT; float clon = OWM_LON; String ccity=""; load_location(clat, clon, ccity);
  j += ",\"lat\":"; j += String(clat, 4);
  j += ",\"lon\":"; j += String(clon, 4);
  if (ccity.length()) { j += ",\"city\":\""; String tmp=ccity; tmp.replace("\"","\\\""); j += tmp; j += "\""; }
  j += "},\"names\":[";
  for (int i = 0; i < 10; ++i) {
    String nm = g_sensorNames[i]; nm.replace("\"","\\\"");
    j += "\""; j += nm; j += "\"";
    if (i < 9) j += ",";
  }
  j += "],\"pumpEnabled\":[";
  for (int i = 0; i < 4; ++i) { j += (g_persist.enabled[i] ? "1" : "0"); if (i < 3) j += ","; }
  j += "],\"pumpPower\":[";
  for (int i = 0; i < 4; ++i) {
    int pct = g_persist.enabled[i] ? (g_persist.manual[i] ? (int)g_persist.overridePct[i] : pwmToPercent(g_pumpPWM[i])) : 0;
    j += pct; if (i < 3) j += ",";
  }
  j += "],\"pumpManual\":[";
  for (int i = 0; i < 4; ++i) { j += (g_persist.manual[i] ? "1":"0"); if (i < 3) j += ","; }
  j += "],\"pumpOverride\":[";
  for (int i = 0; i < 4; ++i) { j += (int)g_persist.overridePct[i]; if (i < 3) j += ","; }
  j += "],\"settings\":{";
  j += "\"maxTemp\":"; j += String(maxPumpTemp, 1);
  j += "},\"cutoffPct\":[";
  for (int i=0;i<4;i++){ 
    j += (int)g_persist.cutoffPct[i]; 
    if (i<3) j += ","; 
  }
  Serial.printf("[Web] Sending cutoff values: %d,%d,%d,%d\n", 
                g_persist.cutoffPct[0], g_persist.cutoffPct[1], 
                g_persist.cutoffPct[2], g_persist.cutoffPct[3]);
  j += "],\"lockoutMs\":[";
  unsigned long nowMs = millis();
  for (int i=0;i<4;i++){ unsigned long rem=0; if (g_lockoutUntilMs[i] && (int32_t)(g_lockoutUntilMs[i]-nowMs)>0) rem=(unsigned long)(g_lockoutUntilMs[i]-nowMs); j += rem; if (i<3) j += ","; }
  j += "],\"groupAvg\":[";
  for (int i=0;i<4;i++){ j += (int)groupAverageMoisture(i); if (i<3) j += ","; }
  j += "],\"durations\":{\"lockoutMs\":"; j += (unsigned long)g_lockoutDurationMs;
  j += ",\"manualMs\":"; j += (unsigned long)g_manualDurationMs; j += "}}";
  return j;
}

static void handleRoot() { server.send_P(200, "text/html", INDEX_HTML2); }

static void handleData() {
  fetchWeatherIfDue();
  fetchRainForecastIfDue();
  server.send(200, "application/json", jsonData());
}

static void handleNamesGet() { server.send(200, "text/plain", names_csv(g_sensorNames)); }

static void handleNamesPost() {
  for (int i=0;i<10;i++) {
    String key = "name" + String(i);
    int idx = -1;
    for (uint8_t a=0; a<server.args(); ++a) if (server.argName(a) == key) { idx = a; break; }
    if (idx >= 0) g_sensorNames[i] = server.arg(idx);
  }
  save_names(g_sensorNames);
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// ---- Protected routes (Basic Auth) ----
static void handleSetPump() {
  if (!checkAuth(server)) return;
  if (!server.hasArg("pump") || !server.hasArg("state")) { server.send(400,"text/plain","Bad Request"); return; }
  int pump = server.arg("pump").toInt();
  int state = server.arg("state").toInt();
  if (pump < 0 || pump > 3) { server.send(400,"text/plain","Invalid pump"); return; }
  g_persist.enabled[pump] = (state == 1);
  save_pump_enabled(pump, g_persist.enabled[pump]);
  server.send(200, "text/plain", "OK");
}

static void handleSetPumpOverride() {
  if (!checkAuth(server)) return;
  if (!server.hasArg("pump") || !server.hasArg("manual") || !server.hasArg("value")) {
    server.send(400,"text/plain","Bad Request"); return;
  }
  int pump = server.arg("pump").toInt();
  int man  = server.arg("manual").toInt();
  int val  = server.arg("value").toInt();
  if (pump < 0 || pump > 3) { server.send(400,"text/plain","Invalid pump"); return; }
  if (val < 0) val = 0; if (val > 100) val = 100;

  g_persist.manual[pump] = (man == 1);
  g_persist.overridePct[pump] = (uint8_t)val;
  if (g_persist.manual[pump]) g_manualStartMs[pump] = millis(); else g_manualStartMs[pump] = 0;
  save_pump_override(pump, g_persist.manual[pump], g_persist.overridePct[pump]);
  server.send(200, "text/plain", "OK");
}

void web_setup() {
  load_names(g_sensorNames);
  load_pumps(g_persist);
  // Ensure cutoff defaults are set after loading pump data
  ensureCutoffDefaults();
  // loadSettings() is called in main.cpp before web_setup()

  server.on("/", handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/names", HTTP_GET, handleNamesGet);
  server.on("/names", HTTP_POST, handleNamesPost);
  server.on("/setPump", HTTP_GET, handleSetPump);
  server.on("/setPumpOverride", HTTP_GET, handleSetPumpOverride);
  server.on("/setCutoff", HTTP_GET, [](){
    if (!checkAuth(server)) return;
    if (!server.hasArg("pump") || !server.hasArg("value")) { server.send(400, "text/plain", "Bad Request"); return; }
    int pump = server.arg("pump").toInt();
    int val  = server.arg("value").toInt();
    if (pump < 0 || pump > 3) { server.send(400, "text/plain", "Invalid pump"); return; }
    if (val < 0) val = 0; if (val > 100) val = 100;
    
    Serial.printf("[Web] Setting pump %d cutoff to %d%%\n", pump, val);
    g_persist.cutoffPct[pump] = (uint8_t)val;
    save_cutoff_pct(pump, (uint8_t)val);
    Serial.printf("[Web] Pump %d cutoff saved: %d%%\n", pump, g_persist.cutoffPct[pump]);
    
    server.send(200, "text/plain", "OK");
  });

  // set location (lat,lon,city) protected
  server.on("/setLocation", HTTP_GET, [](){
    if (!checkAuth(server)) return;
    if (!server.hasArg("lat") || !server.hasArg("lon")) { server.send(400, "text/plain", "Bad Request"); return; }
    float lat = server.arg("lat").toFloat();
    float lon = server.arg("lon").toFloat();
    String city = server.hasArg("city") ? server.arg("city") : String("");
    save_location(lat, lon, city);
    // Immediately refresh weather for the new location
    forceFetchWeather();
    forceFetchRain();
    server.send(200, "text/plain", "OK");
  });

  // adjustable durations (protected)
  server.on("/setDurations", HTTP_GET, [](){
    if (!checkAuth(server)) return;
    if (!server.hasArg("lockout") || !server.hasArg("manual")) { server.send(400, "text/plain", "Bad Request"); return; }
    uint32_t lo = (uint32_t) server.arg("lockout").toInt();
    uint32_t ma = (uint32_t) server.arg("manual").toInt();
    if (lo < 60000UL) lo = 60000UL; // lower bound 1 minute
    if (ma < 60000UL) ma = 60000UL;
    g_lockoutDurationMs = lo;
    g_manualDurationMs  = ma;
    save_durations(lo, ma);
    server.send(200, "text/plain", "OK");
  });

  // new: expose max-temp control (no auth here so your UI can call it easily; add auth if you prefer)
  server.on("/setMaxTemp", HTTP_GET, handleSetMaxTemp);

  server.begin();
}

void web_poll() { server.handleClient(); }