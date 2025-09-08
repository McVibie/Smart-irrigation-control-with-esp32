// pumps.cpp
#include "pumps.h"
#include "config.h"
#include "storage.h"

volatile int   g_soil[10] = {0};
volatile float g_tAir=NAN, g_hAir=NAN, g_vBus=NAN, g_i_mA=NAN, g_p_mW=NAN;

PumpPersist g_persist;
int   g_pumpPWM[4] = {0,0,0,0};
uint8_t g_pumpPctOverride[4] = {0,0,0,0};
uint32_t g_manualStartMs[4] = {0,0,0,0};
uint32_t g_lockoutUntilMs[4] = {0,0,0,0};
uint32_t g_lockoutDurationMs = MOISTURE_LOCKOUT_MS;
uint32_t g_manualDurationMs  = MANUAL_OVERRIDE_MAX_MS;

int moistureToPWM(int moisture) { // 0% -> 255 (dry), 100% -> 0 (wet)
  int pwm = map(moisture, 0, 100, 255, 0);
  return constrain(pwm, 0, 255);
}
int pwmToPercent(int pwm) {
  return (pwm * 100 + 127) / 255;
}

void ensureCutoffDefaults() {
  // If any cutoff is zero (unset) or invalid, apply DEFAULT_CUTOFF_PCT and persist
  bool needSave[4] = {false,false,false,false};
  Serial.println("[Cutoff] Checking cutoff defaults...");
  for (int i=0;i<4;i++) {
    Serial.printf("[Cutoff] Pump %d: current=%d, default=%d\n", i, g_persist.cutoffPct[i], DEFAULT_CUTOFF_PCT);
    // Check if cutoff is uninitialized (0) or invalid (>100)
    if (g_persist.cutoffPct[i] == 0 || g_persist.cutoffPct[i] > 100) { 
      g_persist.cutoffPct[i] = DEFAULT_CUTOFF_PCT; 
      needSave[i] = true; 
      Serial.printf("[Cutoff] Pump %d: setting to default %d\n", i, DEFAULT_CUTOFF_PCT);
    }
  }
  // Save any changes to persistent storage
  for (int i=0;i<4;i++) if (needSave[i]) {
    save_cutoff_pct(i, g_persist.cutoffPct[i]);
    Serial.printf("[Cutoff] Pump %d: saved to storage\n", i);
  }
}

void computeAutoPWM(int out[4]) {
  out[0] = moistureToPWM((g_soil[0] + g_soil[1]) / 2);
  out[1] = moistureToPWM((g_soil[2] + g_soil[3]) / 2);
  out[2] = moistureToPWM((g_soil[4] + g_soil[5] + g_soil[6]) / 3);
  out[3] = moistureToPWM((g_soil[7] + g_soil[8] + g_soil[9]) / 3);
}

void applyFinalPWM(const int autoPWM[4]) {
  // Lockout logic based on moisture cutoff
  uint8_t groupAvg[4] = {0,0,0,0};
  groupAvg[0] = (uint8_t)((g_soil[0] + g_soil[1]) / 2);
  groupAvg[1] = (uint8_t)((g_soil[2] + g_soil[3]) / 2);
  groupAvg[2] = (uint8_t)((g_soil[4] + g_soil[5] + g_soil[6]) / 3);
  groupAvg[3] = (uint8_t)((g_soil[7] + g_soil[8] + g_soil[9]) / 3);

  const uint32_t nowMs = millis();

  for (int i=0;i<4;i++) {
    // Trigger lockout if avg >= cutoff
    uint8_t cutoff = g_persist.cutoffPct[i] ? g_persist.cutoffPct[i] : DEFAULT_CUTOFF_PCT;
    if (groupAvg[i] >= cutoff) {
      g_lockoutUntilMs[i] = nowMs + g_lockoutDurationMs;
    }

    // Determine base PWM from manual/auto
    int pwm = g_persist.manual[i] ? (int)(g_persist.overridePct[i] * 255 / 100) : autoPWM[i];

    // Apply enable gate
    if (!g_persist.enabled[i]) pwm = 0;

    // Apply lockout gate
    if (g_lockoutUntilMs[i] != 0 && (int32_t)(g_lockoutUntilMs[i] - nowMs) > 0) {
      pwm = 0;
    }

    g_pumpPWM[i] = pwm;
  }
}

void updateSafetyTimers() {
  const uint32_t nowMs = millis();
  // Manual override auto-return after configured duration
  for (int i=0;i<4;i++) {
    if (g_persist.manual[i]) {
      // If manual start is unknown, initialize now
      if (g_manualStartMs[i] == 0) {
        g_manualStartMs[i] = nowMs;
      }
      if ((uint32_t)(nowMs - g_manualStartMs[i]) >= g_manualDurationMs) {
        g_persist.manual[i] = false; // return to auto
        g_manualStartMs[i] = 0;
        // Persist manual state change; keep same overridePct
        save_pump_override(i, false, g_persist.overridePct[i]);
      }
    } else {
      g_manualStartMs[i] = 0; // reset tracker
    }
  }
}

uint8_t groupAverageMoisture(int pumpIdx) {
  switch (pumpIdx) {
    case 0: return (uint8_t)((g_soil[0] + g_soil[1]) / 2);
    case 1: return (uint8_t)((g_soil[2] + g_soil[3]) / 2);
    case 2: return (uint8_t)((g_soil[4] + g_soil[5] + g_soil[6]) / 3);
    case 3: return (uint8_t)((g_soil[7] + g_soil[8] + g_soil[9]) / 3);
    default: return 0;
  }
}