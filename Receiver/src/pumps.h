// pumps.h
#pragma once
#include <Arduino.h>
#include "storage.h"

struct Telemetry {
  int soil[10] = {0};
  float tAir = NAN, hAir = NAN;
  float vBus = NAN, i_mA = NAN, p_mW = NAN;
};

extern volatile int   g_soil[10];
extern volatile float g_tAir, g_hAir, g_vBus, g_i_mA, g_p_mW;

extern PumpPersist g_persist;
extern int   g_pumpPWM[4];   // 0..255
extern uint8_t g_pumpPctOverride[4]; // 0..100 cache
extern uint32_t g_manualStartMs[4];   // millis when manual was enabled (0 if none)
extern uint32_t g_lockoutUntilMs[4];  // millis until which pump is locked out (0 if none)
extern uint32_t g_lockoutDurationMs;  // configurable; default from config
extern uint32_t g_manualDurationMs;   // configurable; default from config

// Battery monitoring variables (accessible to other modules)
extern uint32_t g_lastBatteryCheck;
extern uint32_t g_lastBatteryAlert;
extern bool g_batteryAlertSent;
extern float g_batteryThreshold;

int  moistureToPWM(int moisture);
int  pwmToPercent(int pwm);
void ensureCutoffDefaults();
void computeAutoPWM(int out[4]);
void applyFinalPWM(const int autoPWM[4]); // respects enable + manual + lockout
void updateSafetyTimers();                 // manual auto-return + expose lockout expiry
uint8_t groupAverageMoisture(int pumpIdx); // helper for UI/status