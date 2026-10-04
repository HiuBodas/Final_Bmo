// =============================================================================
// TimeMenu.h - Suite Alat Waktu BMO: Stopwatch, Timer, & Pomodoro
// 100% Vektor Prosedural Tanpa Bitmaps (OLED 128x64)
// Navigasi: Sensor 2 (GPIO 21) | Kontrol/Aksi: Sensor 1 (GPIO 20)
// =============================================================================
#pragma once
#include <Arduino.h>

enum TimeSubState : uint8_t {
  TIME_SUB_MENU = 0,    // Submenu pilihan: 1. Stopwatch, 2. Timer, 3. Pomodoro, 4. Kembali
  TIME_SUB_STOPWATCH,   // Fitur Stopwatch (Start / Pause / Reset)
  TIME_SUB_TIMER,       // Fitur Countdown Timer (Preset 1, 3, 5, 10, 15, 25 min)
  TIME_SUB_POMODORO     // Fitur Podomoro (25 min Fokus, 5 min Istirahat)
};

void timeMenuInit();
void timeMenuOpen();
void timeMenuClose();
bool timeMenuIsActive();
void timeMenuUpdate();
TimeSubState timeMenuGetSubState();
