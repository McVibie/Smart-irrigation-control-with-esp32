// weather.h
#pragma once
#include <Arduino.h>
#include "secrets.h"

extern String w_city, w_desc;
extern float  w_temp, w_hum;
extern bool   g_rainNext24h;

void weather_setup();
void fetchWeatherIfDue();
void fetchRainForecastIfDue();
void forceFetchWeather();
void forceFetchRain();  // optional