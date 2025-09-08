// lora_link.cpp
#include "lora_link.h"

SemaphoreHandle_t g_loraMutex = nullptr;

void lora_setup() {
  SPI.begin(MY_LORA_SCK, MY_LORA_MISO, MY_LORA_MOSI, MY_LORA_SS);
  LoRa.setPins(MY_LORA_SS, MY_LORA_RST, MY_LORA_IRQ);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("[LoRa] init failed!");
    while (true) delay(100);
  }
  // PHY must match the TX
  LoRa.setSPIFrequency(4000000);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(8);
  LoRa.enableCrc();
  LoRa.setSyncWord(0x12);
  LoRa.setTxPower(14);
  Serial.println("[LoRa] ready");

  g_loraMutex = xSemaphoreCreateMutex();
}

bool lora_send_csv(const String& s) {
  if (!g_loraMutex) return false;
  if (xSemaphoreTake(g_loraMutex, pdMS_TO_TICKS(20)) != pdTRUE) return false;
  LoRa.beginPacket();
  LoRa.print(s);
  LoRa.endPacket();
  xSemaphoreGive(g_loraMutex);
  return true;
}

bool lora_read_packet(String& out) {
  if (!g_loraMutex) return false;
  bool read = false;
  if (xSemaphoreTake(g_loraMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    int pkt = LoRa.parsePacket();
    if (pkt) {
      out.reserve(pkt);
      while (LoRa.available()) out += (char)LoRa.read();
      read = true;
    }
    xSemaphoreGive(g_loraMutex);
  }
  return read;
}