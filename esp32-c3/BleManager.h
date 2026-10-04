// =============================================================================
// BleManager.h - Modul Komunikasi Bluetooth Low Energy (BLE) & Notifikasi Real-time
// 100% Vektor Prosedural Tanpa Bitmaps untuk Layar OLED SSD1306 (128x64)
// Mendukung smartphone Android & iPhone (iOS) via Nordic UART Service (NUS)
// =============================================================================
#pragma once
#include <Arduino.h>
#include "Moods.h"
#include "Touch.h"

// Konfigurasi Target OS
enum BleTargetOS : uint8_t {
  BLE_OS_NONE = 0,
  BLE_OS_ANDROID,
  BLE_OS_IOS
};

// Struktur Data Notifikasi Masuk
struct BleNotification {
  char app[24];
  char sender[28];
  char body[96];
  uint32_t receivedMs;
  bool valid;
};

// Inisialisasi & Pengaturan BLE
void bleInit();
void bleStart(BleTargetOS target);
void bleStop();
bool bleIsActive();
bool bleIsConnected();
BleTargetOS bleGetTargetOS();
const char* bleGetDeviceName();
void bleSend(const String& msg);

// Notifikasi Real-time (Interupsi Layar Ekspresi)
bool bleNotifIsActive();
void bleNotifDismiss();
void bleNotifUpdate();
void parseIncomingBleMessage(const String& raw);

// Triggers Ekspresi & Sentuhan dari APK (Thread-safe antar Core)
bool bleHasPendingAction();
bool bleHasPendingMood(Mood &m);
bool bleHasPendingReaction(TouchKind &k);

// Menu Sub-Halaman BLE
void bleMenuUpdate();

// Riwayat Notifikasi Masuk (Inbox 5 Pesan Terakhir)
uint8_t bleGetInboxCount();
const BleNotification* bleGetInboxItem(uint8_t index); // 0 = notifikasi terbaru
void bleClearInbox();
