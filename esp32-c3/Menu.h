// =============================================================================
// Menu.h - Sistem Navigasi Bertingkat BMO (100% Vektor Prosedural Tanpa Bitmaps)
// Tingkat 1: Menu Utama 8 Pilihan
//   1. PILIH EKSPRESI
//   2. WAKTU & TIMER
//   3. BMO TAMAGOTCHI
//   4. BMO ARCADE GAME
//   5. RIWAYAT NOTIF
//   6. BLUETOOTH & NOTIF
//   7. PENGATURAN BMO
//   8. KEMBALI KE HOME
// =============================================================================
#pragma once
#include <Arduino.h>
#include "Moods.h"

enum MenuState : uint8_t {
  MENU_STATE_HOME = 0,         // Home: Wajah BMO berekspresi & interaksi sensor wajah normal
  MENU_STATE_MAIN,             // Menu Tingkat Atas 8 Pilihan
  MENU_STATE_SUB_EKSPRESI,     // Submenu 1: Daftar teks 26 ekspresi BMO
  MENU_STATE_SUB_TIME,         // Submenu 2: Suite Waktu (Stopwatch, Timer, Pomodoro)
  MENU_STATE_SUB_TAMAGOTCHI,   // Submenu 3: BMO Tamagotchi Virtual Pet
  MENU_STATE_SUB_GAME,         // Submenu 4: BMO Arcade Jump Runner Game
  MENU_STATE_SUB_INBOX,        // Submenu 5: Riwayat Pesan Notifikasi Masuk
  MENU_STATE_SUB_BLE,          // Submenu 6: Koneksi Bluetooth & Notifikasi Real-time
  MENU_STATE_SUB_SETTINGS      // Submenu 7: Pengaturan BMO (Kontras, Flip, Kepribadian)
};

void menuInit();
bool menuIsActive();
MenuState menuGetState();
void menuOpen();
void menuClose();
void menuUpdate();

// Status mode penguncian mood yang dipilih
bool menuIsLocked();
Mood menuGetLockedMood();
void menuUnlock();

// Pengaturan Kepribadian (0 = Siklus 26 Mood Rutin, 1 = Mochi Personality Engine)
bool menuIsMochiEngineEnabled();
void menuSetMochiEngineEnabled(bool enabled);
