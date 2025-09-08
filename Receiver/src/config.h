#pragma once
#include <Arduino.h>

// --------- Board Pins (TTGO LoRa32 v1/v2.x) ----------
// Renamed to avoid conflicts with pins_arduino.h macros
static const int MY_LORA_SCK  = 5;   // GPIO5  - SX1276 SCK
static const int MY_LORA_MISO = 19;  // GPIO19 - SX1276 MISO
static const int MY_LORA_MOSI = 27;  // GPIO27 - SX1276 MOSI
static const int MY_LORA_SS   = 18;  // GPIO18 - SX1276 CS
static const int MY_LORA_RST  = 23;  // GPIO23 - SX1276 RST
static const int MY_LORA_IRQ  = 26;  // GPIO26 - SX1276 IRQ
static const long LORA_FREQ   = 866E6;

// OLED (I2C on default 21/22)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Weather refresh intervals
static const uint32_t OWM_REFRESH_MS  = 10UL * 60UL * 1000UL; // 10 min
static const uint32_t RAIN_REFRESH_MS = 15UL * 60UL * 1000UL; // 15 min
static const float OWM_LAT = 45.7142f; // Velika Gorica
static const float OWM_LON = 16.0752f;

// Time zone offset (UTC+2 for Croatia)
static const int TIMEZONE_OFFSET_HOURS = 2;

// LoRa TX rate-limit for control messages
static const uint32_t MIN_SEND_INTERVAL_MS = 60; // ~16 Hz max

// Soil groups (same logic as before): 
// g0 = (1,2), g1 = (3,4), g2 = (5,6,7), g3 = (8,9,10)

// Web server settings
static const uint16_t WEB_PORT = 80;

// Watchdog (task feed period must be < WDT_TIMEOUT_MS)
static const uint32_t WDT_TIMEOUT_MS = 8000;

// Task stack sizes (tuned, not tiny)
static const uint32_t STACK_SMALL  = 4096;
static const uint32_t STACK_MEDIUM = 6144;
static const uint32_t STACK_LARGE  = 8192;

// Irrigation safety controls
// Moisture lockout: when group average moisture >= cutoff, force pump OFF and lock out for a period
static const uint32_t MOISTURE_LOCKOUT_MS = 10UL * 60UL * 1000UL; // 10 minutes

// Manual override auto-return duration
static const uint32_t MANUAL_OVERRIDE_MAX_MS = 20UL * 60UL * 1000UL; // 20 minutes

// Default per-pump cutoff percentage (can be overridden per pump via UI and NVS)
static const uint8_t DEFAULT_CUTOFF_PCT = 85; // percent (0..100)

