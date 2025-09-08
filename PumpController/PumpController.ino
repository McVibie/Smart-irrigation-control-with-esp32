// PUMP ESP32 UART RECEIVER + PWM control for 4 pumps (DRV8833 single-ended)
// Target: Arduino Nano ESP32 (ESP32-S3) — uses analogWrite() API (per-pin config for core 3.3.0)
#include <Arduino.h>

// ---------------- UART to TTGO ----------------
#define UART_RX_PIN 18      // connect to TTGO TX (GPIO4)
#define UART_TX_PIN 17      // optional: echo/ACK back to TTGO (to its GPIO39 if wired)
#define UART_BAUD   115200

// ---------------- Pump PWM --------------------
static const uint8_t PUMP_PINS[4] = {5, 6, 7, 8};   // Nano ESP32 (S3) usable pins
#define LEDC_FREQ_HZ   5000     // target PWM freq
#define LEDC_RES_BITS  8        // 0..255 duty

// Optional DRV8833 STBY pin (tie to 3V3 if not used)
#define DRV_STBY_PIN   -1       // set to a valid GPIO if you wired STBY; otherwise keep -1

// Safety: if no new command within this many ms, stop all pumps
#define CMD_TIMEOUT_MS 5000

// Simple line buffer
static String rxLine;
static uint32_t lastCmdMillis = 0;

// Parse "P,<d0>,<d1>,<d2>,<d3>"
static bool parsePLine(const String& line, int& d0, int& d1, int& d2, int& d3) {
  if (!line.startsWith("P,")) return false;

  int c1 = line.indexOf(',', 2);
  if (c1 < 0) return false;
  int c2 = line.indexOf(',', c1 + 1);
  if (c2 < 0) return false;
  int c3 = line.indexOf(',', c2 + 1);
  if (c3 < 0) return false;

  d0 = line.substring(2,       c1).toInt();
  d1 = line.substring(c1 + 1,  c2).toInt();
  d2 = line.substring(c2 + 1,  c3).toInt();
  d3 = line.substring(c3 + 1).toInt();

  auto clamp = [](int v){ return v < 0 ? 0 : (v > 255 ? 255 : v); };
  d0 = clamp(d0); d1 = clamp(d1); d2 = clamp(d2); d3 = clamp(d3);
  return true;
}

// Apply duties (0..255) to the 4 PWM pins
static void writePumpDuties(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) {
  analogWrite(PUMP_PINS[0], d0);
  analogWrite(PUMP_PINS[1], d1);
  analogWrite(PUMP_PINS[2], d2);
  analogWrite(PUMP_PINS[3], d3);
}

// Stop all pumps
static void stopAll() {
  writePumpDuties(0,0,0,0);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[PUMP-ESP32] boot (Nano ESP32 / S3)");

  // Start UART2
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial.printf("[UART] RX=%d  TX=%d @ %d baud\n", UART_RX_PIN, UART_TX_PIN, UART_BAUD);
  Serial.println("[INFO] Connect TTGO GPIO4 -> this RX pin, and GND <-> GND");
  Serial.println("[INFO] Expect lines like: P,120,0,200,60");

  // Optional STBY
  if (DRV_STBY_PIN >= 0) {
    pinMode(DRV_STBY_PIN, OUTPUT);
    digitalWrite(DRV_STBY_PIN, HIGH);
    Serial.printf("[DRV8833] STBY pin %d set HIGH\n", DRV_STBY_PIN);
  } else {
    Serial.println("[DRV8833] STBY not controlled by MCU (tie STBY to 3V3 or ensure it's HIGH).");
  }

  // PWM init via analogWrite() API — per-pin on ESP32 core 3.3.0
  for (uint8_t i = 0; i < 4; ++i) {
    pinMode(PUMP_PINS[i], OUTPUT);
    analogWriteResolution(PUMP_PINS[i], LEDC_RES_BITS);   // set 8-bit resolution per pin
    analogWriteFrequency(PUMP_PINS[i], LEDC_FREQ_HZ);     // set 5 kHz per pin
    analogWrite(PUMP_PINS[i], 0);                         // start stopped
  }

  stopAll();
  lastCmdMillis = millis();
}

void loop() {
  // Read lines from UART2
  while (Serial2.available()) {
    char c = (char)Serial2.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (rxLine.length()) {
        int d0=0,d1=0,d2=0,d3=0;
        if (parsePLine(rxLine, d0, d1, d2, d3)) {
          Serial.printf("[RX] %s  -> duty: %d,%d,%d,%d\n",
                        rxLine.c_str(), d0, d1, d2, d3);
          writePumpDuties((uint8_t)d0, (uint8_t)d1, (uint8_t)d2, (uint8_t)d3);
          lastCmdMillis = millis();
          if (UART_TX_PIN >= 0) Serial2.println("ACK");
        } else {
          Serial.printf("[RX] %s  -> invalid\n", rxLine.c_str());
        }
        rxLine = "";
      }
    } else {
      rxLine += c;
      if (rxLine.length() > 120) rxLine = ""; // guard
    }
  }

  // Fail-safe timeout
  if (millis() - lastCmdMillis > CMD_TIMEOUT_MS) {
    stopAll();
  }

  // Optional: forward what you type in Serial Monitor to TTGO (debug)
  while (Serial.available()) {
    char c = (char)Serial.read();
    Serial2.write(c);
  }
}