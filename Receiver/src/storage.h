// storage.h
#pragma once
#include <Arduino.h>
#include <Preferences.h>

struct PumpPersist {
  bool enabled[4]     = {true,true,true,true};
  bool manual[4]      = {false,false,false,false};
  uint8_t overridePct[4] = {0,0,0,0};
  uint8_t cutoffPct[4]   = {0,0,0,0}; // per-pump moisture cutoff (0..100)
};

extern Preferences g_prefs;

void storage_begin();
void load_names(String names[10]);
void save_names(const String names[10]);
void load_pumps(PumpPersist& p);
void save_pump_enabled(int idx, bool st);
void save_pump_override(int idx, bool manual, uint8_t pct);
void save_cutoff_pct(int idx, uint8_t pct);
void load_durations(uint32_t& lockoutMs, uint32_t& manualMs);
void save_durations(uint32_t lockoutMs, uint32_t manualMs);
// Weather/location persistence
void load_location(float& lat, float& lon, String& city);
void save_location(float lat, float lon, const String& city);
String names_csv(const String names[10]);
void csv_to_names(const String& s, String out[10], char sep='|');