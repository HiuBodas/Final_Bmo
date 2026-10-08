// =============================================================================
// WeatherManager.h - Modul Cuaca & Suhu HUD ESP32-C3 (Seksi 2.2)
// Mengambil data cuaca publik ringan secara non-blocking via HTTP
// =============================================================================
#pragma once
#include <Arduino.h>

enum WeatherKind : uint8_t {
  WEATHER_SUN = 0,     // Cerah / Panas
  WEATHER_CLOUD,       // Berawan / Mendung
  WEATHER_RAIN,        // Hujan
  WEATHER_THUNDER      // Badai / Petir
};

void weatherInit();
void weatherUpdate(); // Dipanggil di dalam FreeRTOS Background Task
bool weatherHasData();

int         weatherGetTemp();
int         weatherGetHumidity();
String      weatherGetCondition();
WeatherKind weatherGetKind();
String      weatherGetCity();
void        weatherSetCity(const String& city);
