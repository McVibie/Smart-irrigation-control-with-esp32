// lora_link.h
#pragma once
#include <Arduino.h>
#include <LoRa.h>
#include <SPI.h>
#include "config.h"

extern SemaphoreHandle_t g_loraMutex;

void lora_setup();
bool lora_send_csv(const String& s);   // returns true if sent
bool lora_read_packet(String& out);    // returns true if packet read