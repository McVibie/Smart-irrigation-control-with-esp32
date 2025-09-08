#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include <DHT.h>

// ---------- DEBUG SWITCHES ----------
#define DEBUG_LORA_SEND   1
#define DEBUG_LORA_RX     1
#define DEBUG_UART_FORWARD 1
#define DEBUG_RATE        1     // print every Nth packet (keep at 1 for now)

// ---------- OLED / I2C ----------
#define I2C_SDA 21
#define I2C_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- LoRa (TTGO LoRa32 T3 v1.6.x) ----------
#define LORA_SCK  5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS   18
#define LORA_RST  23
#define LORA_IRQ  26
#define LORA_FREQ 866E6

// ---------- CD74HC4067 multiplexer ----------
#define MUX_SIG 36   // ADC1_CH0
#define MUX_S0  14
#define MUX_S1  12
#define MUX_S2  13
#define MUX_S3  15

// ---------- DHT11 ----------
#define DHTPIN   25
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);

// ---------- INA219 ----------
Adafruit_INA219 ina219;
bool inaOK = false;

// ---------- Calibration ----------
static const int AIR_VALUE   = 3360;   // dry
static const int WATER_VALUE = 1040;   // wet

// ---------- Globals ----------
volatile int   soilPct[10] = {0};
volatile float tempC = NAN, hum = NAN;
volatile float busV = NAN, current_mA = NAN, power_mW = NAN;

// Store most-recent pump duties received from receiver (0..255)
volatile uint8_t rxPumpDuty[4] = {0,0,0,0};

// ---------- Mutexes ----------
SemaphoreHandle_t sampleMutex;
SemaphoreHandle_t i2cMutex;
SemaphoreHandle_t loraMutex;

// ---------- UART2 Pins to Pump ESP32 (this board = TTGO) ----------
#define UART_TX 4    // TTGO TX2 (GPIO4) → PumpESP RX
#define UART_RX 39   // TTGO RX2 (GPIO39, input-only) ← PumpESP TX (optional)
#define UART_BAUD 115200

// ===================== Helpers =====================
static inline void muxSelect(uint8_t ch) {
  digitalWrite(MUX_S0, (ch >> 0) & 0x01);
  digitalWrite(MUX_S1, (ch >> 1) & 0x01);
  digitalWrite(MUX_S2, (ch >> 2) & 0x01);
  digitalWrite(MUX_S3, (ch >> 3) & 0x01);
  delayMicroseconds(20);
}

int readSoilPercent(uint8_t channel) {
  muxSelect(channel);
  const uint8_t N = 8;
  uint32_t acc = 0;
  for (uint8_t i = 0; i < N; i++) { acc += analogRead(MUX_SIG); delayMicroseconds(100); }
  int raw = acc / N;

  long num = (long)(raw - AIR_VALUE) * 100L;
  long den = (long)(WATER_VALUE - AIR_VALUE);
  int pct = (int)((float)num / (float)den);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

// duty 0..255 -> percent 0..100 (rounded)
static inline int dutyToPct(uint8_t d){ return (int)((d * 100 + 127) / 255); }

// Parse "d0,d1,d2,d3" into rxPumpDuty[]
static void parsePumpCSV_toDuty(const String& s) {
  uint8_t vals[4] = {0,0,0,0};
  int idx = 0, start = 0;
  while (idx < 4) {
    int comma = s.indexOf(',', start);
    String tok = (comma == -1) ? s.substring(start) : s.substring(start, comma);
    tok.trim();
    int v = tok.toInt();
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    vals[idx++] = (uint8_t)v;
    if (comma == -1) break;
    start = comma + 1;
  }
  for (int i = 0; i < 4; ++i) rxPumpDuty[i] = vals[i];
}

// ===================== Tasks =====================
void TaskReadSensors(void *pv);
void TaskSendLoRa(void *pv);
void TaskUpdateOLED(void *pv);
void TaskRecvPWM(void *pv);

// ===================== Setup =====================
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("\n[TTGO BOOT] setup() start");

  Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX, UART_TX);
  Serial.printf("[UART2] RX=%d TX=%d @ %d\n", UART_RX, UART_TX, UART_BAUD);

  // ADC config
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  analogSetPinAttenuation(MUX_SIG, ADC_11db);

  // MUX pins
  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);
  pinMode(MUX_SIG, INPUT);

  // I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  // Mutexes
  i2cMutex    = xSemaphoreCreateMutex();
  sampleMutex = xSemaphoreCreateMutex();
  loraMutex   = xSemaphoreCreateMutex();

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[OLED] not found");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Starting TTGO TX...");
    display.display();
  }

  // INA219
  inaOK = ina219.begin();
  Serial.println(inaOK ? "[INA219] OK" : "[INA219] not found");

  // DHT
  dht.begin();

  // LoRa
  Serial.println("[LoRa] init...");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_IRQ);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("[LoRa] init failed!");
    while (true) { delay(500); Serial.print("!"); }
  }
  LoRa.setSPIFrequency(4000000);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(8);
  LoRa.enableCrc();
  LoRa.setSyncWord(0x12);
  LoRa.setTxPower(14);
  Serial.printf("[LoRa] ready @ %.0f Hz\n", LORA_FREQ);

  // FreeRTOS tasks
  xTaskCreatePinnedToCore(TaskReadSensors, "ReadSensors", 4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(TaskSendLoRa,    "SendLoRa",    4096, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(TaskUpdateOLED,  "OLED",        4096, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(TaskRecvPWM,     "RecvPWM",     4096, nullptr, 1, nullptr, 0);

  Serial.println("[TTGO BOOT] setup() done");
}

void loop() {}

// ===================== Task implementations =====================
void TaskReadSensors(void *pv) {
  for (;;) {
    int localSoil[10];
    for (uint8_t ch = 0; ch < 10; ch++) localSoil[ch] = readSoilPercent(ch);

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    float v = NAN, i = NAN, p = NAN;
    if (inaOK && xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(40)) == pdTRUE) {
      v = ina219.getBusVoltage_V();
      i = ina219.getCurrent_mA();
      p = ina219.getPower_mW();
      xSemaphoreGive(i2cMutex);
    }

    if (xSemaphoreTake(sampleMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      for (int k = 0; k < 10; k++) soilPct[k] = localSoil[k];
      if (!isnan(t)) tempC = t;
      if (!isnan(h)) hum   = h;
      busV = v; current_mA = i; power_mW = p;
      xSemaphoreGive(sampleMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(120));
  }
}

void TaskSendLoRa(void *pv) {
  uint32_t n = 0;
  for (;;) {
    int   s[10];
    float t, h, v, i, p;

    if (xSemaphoreTake(sampleMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      for (int k = 0; k < 10; ++k) s[k] = soilPct[k];
      t = tempC; h = hum; v = busV; i = current_mA; p = power_mW;
      xSemaphoreGive(sampleMutex);
    }

    // CSV: s1..s10,temp,hum,busV,current_mA,power_mW
    String payload;
    payload.reserve(128);
    for (int k = 0; k < 10; ++k) { payload += String(s[k]); payload += ','; }
    payload += String(isnan(t) ? 0.0f : t, 1); payload += ",";
    payload += String(isnan(h) ? 0.0f : h, 0); payload += ",";
    payload += String(isnan(v) ? 0.0f : v, 2); payload += ",";
    payload += String(isnan(i) ? 0.0f : i, 1); payload += ",";
    payload += String(isnan(p) ? 0.0f : p, 1);

    int ret = 0;
    if (xSemaphoreTake(loraMutex, pdMS_TO_TICKS(40)) == pdTRUE) {
      LoRa.idle();
      LoRa.beginPacket();
      LoRa.print(payload);
      ret = LoRa.endPacket();  // 1 on success with most libs
      xSemaphoreGive(loraMutex);
    }

#if DEBUG_LORA_SEND
    if ((n % DEBUG_RATE) == 0) {
      Serial.printf("[TTGO LoRa TX] ret=%d len=%u: %s\n", ret, payload.length(), payload.c_str());
    }
#endif
    n++;
    vTaskDelay(pdMS_TO_TICKS(200)); // 5 Hz
  }
}

void TaskUpdateOLED(void *pv) {
  for (;;) {
    if (!display.getBuffer()) { vTaskDelay(pdMS_TO_TICKS(500)); continue; }

    int   s[10];
    float t = NAN, h = NAN;
    uint8_t d0, d1, d2, d3;

    if (xSemaphoreTake(sampleMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      for (int k = 0; k < 10; ++k) s[k] = soilPct[k];
      t = tempC; h = hum;
      d0 = rxPumpDuty[0]; d1 = rxPumpDuty[1]; d2 = rxPumpDuty[2]; d3 = rxPumpDuty[3];
      xSemaphoreGive(sampleMutex);
    }

    auto cap99 = [](int v){ return (v > 99) ? 99 : (v < 0 ? 0 : v); };
    int P1 = cap99(dutyToPct(d0)), P2 = cap99(dutyToPct(d1));
    int P3 = cap99(dutyToPct(d2)), P4 = cap99(dutyToPct(d3));

    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(60)) == pdTRUE) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);

      const int X1 = 0;    // S1..S5
      const int X2 = 44;   // S6..S10
      const int X3 = 90;   // P1..P4
      const int DY = 10;

      for (int i = 0; i < 5; i++) {
        display.setCursor(X1, i * DY);
        display.printf("S%d:%2d%%", i + 1, cap99(s[i]));
      }
      for (int i = 0; i < 5; i++) {
        display.setCursor(X2, i * DY);
        display.printf("S%d:%2d%%", i + 6, cap99(s[i + 5]));
      }

      display.setCursor(X3, 0 * DY); display.printf("P1:%2d%%", P1);
      display.setCursor(X3, 1 * DY); display.printf("P2:%2d%%", P2);
      display.setCursor(X3, 2 * DY); display.printf("P3:%2d%%", P3);
      display.setCursor(X3, 3 * DY); display.printf("P4:%2d%%", P4);

      display.setCursor(X1, 5 * DY);
      display.print("T:");
      if (isnan(t)) display.print("--.-"); else display.print(t, 1);
      display.print("C H:");
      if (isnan(h)) display.print("--"); else display.print((int)h);
      display.print("%");

      display.display();
      xSemaphoreGive(i2cMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void TaskRecvPWM(void *pv) {
  for (;;) {
    if (xSemaphoreTake(loraMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      int pkt = LoRa.parsePacket();
      if (pkt) {
        String msg;
        while (LoRa.available()) msg += (char)LoRa.read();
        xSemaphoreGive(loraMutex);

#if DEBUG_LORA_RX
        Serial.print("[TTGO LoRa RX] "); Serial.println(msg);
#endif
        // Update local display
        parsePumpCSV_toDuty(msg);

        // Forward to Pump ESP32 over UART2 as: "P,<csv>\n"
        String uartMsg = "P," + msg;
#if DEBUG_UART_FORWARD
        Serial.print("[TTGO UART2 → Pump] "); Serial.println(uartMsg);
#endif
        Serial2.println(uartMsg);
      } else {
        xSemaphoreGive(loraMutex);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}