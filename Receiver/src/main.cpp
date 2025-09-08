#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

#include "config.h"
#include "secrets.h"
#include "weather.h"
#include "lora_link.h"
#include "web_server.h"
#include "pumps.h"
#include "storage.h"
#include "azure_iot.h"


// ------------------- Globals -------------------
Preferences prefs;
float maxPumpTemp = 22.0f; // default

// OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Timing
static uint32_t g_lastSend = 0;

// --- Tasks ---
void TaskWeb(void* pv);
void TaskLoRaRXTX(void* pv);
void TaskOLED(void* pv);
void TaskWeather(void* pv);
void TaskAzure(void* pv);

// ------------------- Settings persistence (fixed) -------------------
static const float DEFAULT_MAX_PUMP_TEMP = 22.0f;

void loadSettings() {
  bool ok = prefs.begin("settings", /*readOnly=*/true);
  if (!ok) {
    // Create namespace and set default
    prefs.end();
    prefs.begin("settings", false);
    prefs.putFloat("maxTemp", DEFAULT_MAX_PUMP_TEMP);
    prefs.end();
    prefs.begin("settings", true);
  }
  maxPumpTemp = prefs.getFloat("maxTemp", DEFAULT_MAX_PUMP_TEMP);
  prefs.end();
  Serial.printf("[Settings] maxPumpTemp=%.1f°C\n", maxPumpTemp);
}

void saveSettings() {
  if (!prefs.begin("settings", false)) {
    Serial.println("[Settings] ERROR: cannot open NVS for write");
    return;
  }
  prefs.putFloat("maxTemp", maxPumpTemp);
  prefs.end();
  Serial.printf("[Settings] saved maxPumpTemp=%.1f°C\n", maxPumpTemp);
}

// ------------------- CSV parsing from LoRa TX -------------------
static void parseTXCSV(const String& s) {
  float vals[16] = {0};
  int idx = 0, start = 0;
  while (idx < 16) {
    int comma = s.indexOf(',', start);
    String tok = (comma == -1) ? s.substring(start) : s.substring(start, comma);
    tok.trim();
    vals[idx++] = tok.toFloat();
    if (comma == -1) break;
    start = comma + 1;
  }
  for (int i = 0; i < 10; ++i) g_soil[i] = constrain((int)round(vals[i]), 0, 100);
  g_tAir = vals[10];
  g_hAir = vals[11];
  g_vBus = vals[12];
  g_i_mA = vals[13];
  g_p_mW = vals[14];
}

// ------------------- LoRa TX rate limiter -------------------
static void sendPumpPWM_rateLimited() {
  uint32_t now = millis();
  if (now - g_lastSend < MIN_SEND_INTERVAL_MS) return;
  g_lastSend = now;
  String controlMsg = String(g_pumpPWM[0]) + "," + String(g_pumpPWM[1]) + "," +
                      String(g_pumpPWM[2]) + "," + String(g_pumpPWM[3]);
  lora_send_csv(controlMsg);
  Serial.println("LoRa -> TX (PWM): " + controlMsg);
}

// ------------------- HTTP handler for max temp -------------------
// GET /setMaxTemp?value=23.5
void handleSetMaxTemp() {
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "Missing value");
    return;
  }
  float v = server.arg("value").toFloat();
  if (v < -20.0f) v = -20.0f;
  if (v > 60.0f)  v = 60.0f;
  maxPumpTemp = v;
  saveSettings();
  server.send(200, "text/plain", "OK");
}

// ------------------- Setup -------------------
void setup() {
  Serial.begin(115200);
  delay(50);
  storage_begin();

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[OLED] not found");
  } else {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("Receiver booting...");
    display.display();
  }

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Connecting WiFi...");
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());

  loadSettings();
  weather_setup();
  lora_setup();
  web_setup();
  // Azure IoT (optional; no-op if secrets are empty)
  azure_iot_begin();

  // Load adjustable durations (override defaults if present)
  uint32_t loMs=0, manMs=0; load_durations(loMs, manMs);
  if (loMs > 0) g_lockoutDurationMs = loMs;
  if (manMs > 0) g_manualDurationMs = manMs;

  // Tasks
  xTaskCreatePinnedToCore(TaskWeb,    "Web",    STACK_SMALL,  nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(TaskLoRaRXTX,"LoRa",   STACK_MEDIUM, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(TaskOLED,   "OLED",   STACK_SMALL,  nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(TaskWeather,"Weather",STACK_SMALL,  nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(TaskAzure,  "Azure",  STACK_MEDIUM,  nullptr, 1, nullptr, 0);
}

void loop() {
  delay(50);
}

// ------------------- Tasks -------------------
void TaskWeb(void* pv) {
  for (;;) {
    web_poll();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void TaskLoRaRXTX(void* pv) {
  for (;;) {
    String msg;
    if (lora_read_packet(msg)) {
      Serial.println("LoRa <- " + msg);
      parseTXCSV(msg);

      // Compute auto & apply
      int autoPWM[4]; 
      computeAutoPWM(autoPWM);
      // Update timers (manual auto-return)
      updateSafetyTimers();
      // Apply considering enable/manual/lockout
      applyFinalPWM(autoPWM);

      // Weather gating
      bool tempOK = (!isnan(w_temp) && w_temp < maxPumpTemp);
      bool weatherAllows = (tempOK && !g_rainNext24h);
      if (!weatherAllows) {
        for (int i=0;i<4;i++) if (!g_persist.manual[i]) g_pumpPWM[i] = 0;
      }

      sendPumpPWM_rateLimited();
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void TaskOLED(void* pv) {
  for (;;) {
    if (!display.getBuffer()) { vTaskDelay(pdMS_TO_TICKS(500)); continue; }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    // Top line: host IP of the web UI
    String ip = WiFi.localIP().toString();
    display.setCursor(0, 0);
    display.print("IP: ");
    display.print(ip);

    // Pump power percentages (computed like the web UI) - 2 pumps per row
    for (int i = 0; i < 4; ++i) {
      int pct = g_persist.enabled[i]
        ? (g_persist.manual[i] ? (int)g_persist.overridePct[i] : pwmToPercent(g_pumpPWM[i]))
        : 0;
      
      int row = i / 2;        // 0,0,1,1 for pumps 1,2,3,4
      int col = i % 2;        // 0,1,0,1 for pumps 1,2,3,4
      int y = 14 + row * 12;  // two rows under IP
      
      if (col == 0) {
        // Left side
        display.setCursor(0, y);
        display.printf("P%d:", i+1);
        display.setCursor(25, y);
        display.printf("%3d%%", pct);
      } else {
        // Right side
        display.setCursor(65, y);
        display.printf("P%d:", i+1);
        display.setCursor(90, y);
        display.printf("%3d%%", pct);
      }
    }

    // Battery voltage display (below pumps)
    if (!isnan(g_vBus)) {
      int y = 14 + 2 * 12; // below the 2 pump rows (not 4 lines)
      display.setCursor(0, y);
      display.print("Battery:");
      display.setCursor(58, y);
      display.printf("%.2fV", g_vBus);
    }

    display.display();
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void TaskWeather(void* pv) {
  for (;;) {
    fetchWeatherIfDue();
    fetchRainForecastIfDue();
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void TaskAzure(void* pv) {
  for(;;){
    if (azure_iot_is_enabled()) {
      azure_iot_loop();
      azure_publish_telemetry();
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
