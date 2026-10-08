// =============================================================================
// NtpClock.h - Modul Jam & Kalender Internet Otomatis ESP32-C3 (Seksi 2.1)
// Sinkronisasi otomatis ke NTP Pool (WIB UTC+7) dan RTC internal ESP32
// =============================================================================
#pragma once
#include <Arduino.h>

void ntpInit();
void ntpStartSync();
bool ntpIsSynced();

void ntpGetTime(int &hour, int &minute, int &second);
String ntpGetTimeStr();         // "14:25:30"
String ntpGetDateStr();         // "08/10/2026"
String ntpGetDayStr();          // "Kamis"
String ntpGetFullDateTimeStr(); // "Kamis, 08/10/2026 14:25:30"
