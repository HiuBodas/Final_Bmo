// =============================================================================
// TimeMenu.h - Suite Alat Waktu BMO: Stopwatch, Timer, & Pomodoro
// 100% Vektor Prosedural Tanpa Bitmaps (OLED 128x64)
// Navigasi: Sensor 2 (GPIO 21) | Kontrol/Aksi: Sensor 1 (GPIO 20)
// =============================================================================
#pragma once
#include <Arduino.h>

enum TimeSubState : uint8_t {
  TIME_SUB_MENU = 0,    // Submenu pilihan
  TIME_SUB_CLOCK,       // Jam Digital NTP & Cuaca HUD (Seksi 2.1 & 2.2)
  TIME_SUB_SPOTIFY,     // Spotify Now Playing Widget (Seksi 2.4)
  TIME_SUB_STOPWATCH,   // Fitur Stopwatch (Start / Pause / Reset)
  TIME_SUB_TIMER,       // Fitur Countdown Timer
  TIME_SUB_POMODORO     // Fitur Podomoro (Fokus / Istirahat)
};

void timeMenuInit();
void timeMenuOpen();
void timeMenuClose();
bool timeMenuIsActive();
void timeMenuUpdate();
TimeSubState timeMenuGetSubState();

// Kontrol Pomodoro Jarak Jauh (Web Dashboard & API)
void     timeMenuStartPomodoro();
void     timeMenuPausePomodoro();
void     timeMenuResetPomodoro();
bool     timeMenuIsPomodoroRunning();
uint32_t timeMenuGetPomodoroRemain();
bool     timeMenuIsPomodoroWork();
uint8_t  timeMenuGetPomodoroCycle();
