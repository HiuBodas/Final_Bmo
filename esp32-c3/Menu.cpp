// =============================================================================
// Menu.cpp - Implementasi Menu Bertingkat BMO (100% Vektor Prosedural Tanpa Bitmaps)
// Tingkat 1: Menu Utama 8 Pilihan
//   1. PILIH EKSPRESI     5. RIWAYAT NOTIF
//   2. WAKTU & TIMER      6. BLUETOOTH & NOTIF
//   3. BMO TAMAGOTCHI     7. PENGATURAN BMO
//   4. BMO ARCADE GAME    8. KEMBALI KE HOME
// =============================================================================
#include "Menu.h"
#include "TimeMenu.h"
#include "Tamagotchi.h"
#include "Game.h"
#include "BleManager.h"
#include "Display.h"
#include "Touch.h"
#include <Preferences.h>

static MenuState s_state          = MENU_STATE_HOME;
static int       s_mainMenuIdx    = 0;
static int       s_mainTopIdx     = 0;
static uint32_t  s_lastActivityMs = 0;

// Submenu 1 (Pilih Ekspresi) Status
static int       s_moodIdx        = 0;
static int       s_moodTopIdx     = 0;
static bool      s_isConfirming   = false;
static uint32_t  s_confirmStartMs = 0;

// Status Mood Terpilih (Locked)
static bool      s_isLocked       = false;
static Mood      s_lockedMood     = MOOD_COOL;

// Submenu 5 (Riwayat Notifikasi Inbox) Status
enum InboxSubView : uint8_t {
  INBOX_VIEW_LIST = 0,
  INBOX_VIEW_DETAIL
};
static InboxSubView s_inboxView        = INBOX_VIEW_LIST;
static int          s_inboxMenuIdx     = 0;
static int          s_inboxTopIdx      = 0;
static int          s_inboxSelectedIdx = 0;
static bool         s_inboxClearedConf = false;
static uint32_t     s_inboxConfStartMs = 0;

// Submenu 7 (Pengaturan BMO) Status
static int          s_settingsIdx      = 0;
static uint8_t      s_contrastLevel    = 2; // 0 = Rendah(35), 1 = Sedang(128), 2 = Maksimal(255)
static bool         s_screenFlipped    = false;
static bool         s_mochiEnabled     = false;
static Preferences  s_cfgPrefs;

// -----------------------------------------------------------------------------
// Word Wrap Helper Khusus Tampilan Layar Detail Notifikasi OLED
// -----------------------------------------------------------------------------
static void wrapInboxBody(const char* input, char lines[4][24]) {
  for (int i = 0; i < 4; ++i) lines[i][0] = '\0';
  if (!input) return;

  int lineIdx = 0;
  int colIdx = 0;
  int inLen = strlen(input);
  int i = 0;

  while (i < inLen && lineIdx < 4) {
    int wordLen = 0;
    while (i + wordLen < inLen && input[i + wordLen] != ' ') {
      wordLen++;
    }

    if (colIdx + wordLen >= 23) {
      if (colIdx > 0) {
        lines[lineIdx][colIdx] = '\0';
        lineIdx++;
        colIdx = 0;
        if (lineIdx >= 4) break;
      }
    }

    for (int w = 0; w < wordLen && colIdx < 23; ++w) {
      lines[lineIdx][colIdx++] = input[i + w];
    }

    i += wordLen;
    if (i < inLen && input[i] == ' ') {
      if (colIdx < 23) lines[lineIdx][colIdx++] = ' ';
      i++;
    }
  }
  if (lineIdx < 4) lines[lineIdx][colIdx] = '\0';
}

// -----------------------------------------------------------------------------
// Inisialisasi Sistem Menu & Pengaturan
// -----------------------------------------------------------------------------
void menuInit() {
  s_state          = MENU_STATE_HOME;
  s_mainMenuIdx    = 0;
  s_mainTopIdx     = 0;
  s_moodIdx        = 0;
  s_moodTopIdx     = 0;
  s_isConfirming   = false;
  s_isLocked       = false;
  s_lockedMood     = MOOD_COOL;
  s_lastActivityMs = millis();

  // Inisialisasi modul terkait
  timeMenuInit();
  tamagotchiInit();
  gameInit();
  bleInit();

  // Muat konfigurasi tersimpan dari Flash NVS
  s_cfgPrefs.begin("bmo_cfg", true);
  s_contrastLevel = s_cfgPrefs.getUChar("contrast", 2);
  s_screenFlipped = s_cfgPrefs.getBool("flip", false);
  s_mochiEnabled  = s_cfgPrefs.getBool("mochi", false);
  s_cfgPrefs.end();

  // Terapkan konfigurasi display langsung
  uint8_t cVal = (s_contrastLevel == 0) ? 35 : ((s_contrastLevel == 1) ? 128 : 255);
  displaySetContrast(cVal);
  displaySetFlip(s_screenFlipped);
}

bool menuIsActive() {
  return s_state != MENU_STATE_HOME;
}

MenuState menuGetState() {
  return s_state;
}

void menuOpen() {
  s_state          = MENU_STATE_MAIN;
  s_mainMenuIdx    = 0;
  s_mainTopIdx     = 0;
  s_isConfirming   = false;
  s_lastActivityMs = millis();
  Serial.println(F("[menu] Masuk ke Menu Utama BMO."));
}

void menuClose() {
  s_state        = MENU_STATE_HOME;
  s_isConfirming = false;
  timeMenuClose();
  tamagotchiClose();
  gameClose();
  Serial.println(F("[menu] Kembali ke Home (Wajah BMO)."));
}

bool menuIsLocked() {
  return s_isLocked;
}

Mood menuGetLockedMood() {
  return s_lockedMood;
}

void menuUnlock() {
  s_isLocked = false;
  Serial.println(F("[menu] Penguncian Mood dilepas. Kembali ke siklus normal."));
}

bool menuIsMochiEngineEnabled() {
  return s_mochiEnabled;
}

void menuSetMochiEngineEnabled(bool enabled) {
  s_mochiEnabled = enabled;
  s_cfgPrefs.begin("bmo_cfg", false);
  s_cfgPrefs.putBool("mochi", s_mochiEnabled);
  s_cfgPrefs.end();
}

// -----------------------------------------------------------------------------
// 1. RENDER & LOGIKA MENU UTAMA (8 PILIHAN DENGAN SCROLL VIEWPORT)
// -----------------------------------------------------------------------------
static const char* const s_mainItems[8] = {
  "PILIH EKSPRESI",
  "WAKTU & TIMER",
  "BMO TAMAGOTCHI",
  "BMO ARCADE GAME",
  "RIWAYAT NOTIF",
  "BLUETOOTH & NOTIF",
  "PENGATURAN BMO",
  "KEMBALI KE HOME"
};
#define MAIN_MENU_COUNT 8

static void updateMainMenu(uint32_t now) {
  // Input Sensor 2 (Navigasi Menu Utama)
  if (g_navPending) {
    NavEvent ev = g_pendingNav;
    g_navPending = false;

    if (ev == NAV_TAP) {
      s_mainMenuIdx = (s_mainMenuIdx + 1) % MAIN_MENU_COUNT;
      s_lastActivityMs = now;

      // Scroll window viewport
      if (s_mainMenuIdx >= s_mainTopIdx + 4) {
        s_mainTopIdx = s_mainMenuIdx - 3;
      } else if (s_mainMenuIdx < s_mainTopIdx) {
        s_mainTopIdx = s_mainMenuIdx;
      }
    } else if (ev == NAV_HOLD || ev == NAV_DOUBLE_TAP) {
      menuClose();
      return;
    }
  }

  // Input Sensor 1 (Aksi / Masuk Submenu)
  if (g_touchPending) {
    g_touchPending = false;
    s_lastActivityMs = now;

    switch (s_mainMenuIdx) {
      case 0: // PILIH EKSPRESI
        s_state = MENU_STATE_SUB_EKSPRESI;
        s_isConfirming = false;
        if (s_moodIdx < s_moodTopIdx) s_moodTopIdx = s_moodIdx;
        else if (s_moodIdx >= s_moodTopIdx + 4) s_moodTopIdx = s_moodIdx - 3;
        return;

      case 1: // WAKTU & TIMER
        s_state = MENU_STATE_SUB_TIME;
        timeMenuOpen();
        return;

      case 2: // BMO TAMAGOTCHI (Virtual Pet Mode)
        s_state = MENU_STATE_SUB_TAMAGOTCHI;
        tamagotchiOpen();
        return;

      case 3: // BMO ARCADE GAME (Mini Jump Runner)
        s_state = MENU_STATE_SUB_GAME;
        gameOpen();
        return;

      case 4: // RIWAYAT NOTIF (Inbox 5 Pesan Terakhir)
        s_state = MENU_STATE_SUB_INBOX;
        s_inboxView        = INBOX_VIEW_LIST;
        s_inboxMenuIdx     = 0;
        s_inboxTopIdx      = 0;
        s_inboxClearedConf = false;
        return;

      case 5: // BLUETOOTH & NOTIF
        s_state = MENU_STATE_SUB_BLE;
        bleMenuOpen();
        return;

      case 6: // PENGATURAN BMO
        s_state = MENU_STATE_SUB_SETTINGS;
        s_settingsIdx = 0;
        return;

      case 7: // KEMBALI KE HOME
      default:
        menuClose();
        return;
    }
  }

  // Timeout Inaktivitas 20 detik
  if (now - s_lastActivityMs > 20000) {
    menuClose();
    return;
  }

  // Render Tampilan Menu Utama
  displayClear();

  // Header Title Bar
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);
  int titleW = fbGetStringWidth("MENU UTAMA");
  fbDrawString((OLED_W - titleW) / 2, 3, "MENU UTAMA", C_WHITE);

  // Render 4 Item dalam Jendela Viewport
  for (int i = 0; i < 4; ++i) {
    int itemIdx = s_mainTopIdx + i;
    if (itemIdx >= MAIN_MENU_COUNT) break;

    int y = 16 + i * 11;
    if (itemIdx == s_mainMenuIdx) {
      fbFillRoundRect(4, y, 114, 11, 2, C_BLACK);
      fbDrawRightArrow(8, y + 3, C_WHITE);
      fbDrawString(18, y + 2, s_mainItems[itemIdx], C_WHITE);
    } else {
      fbDrawString(18, y + 2, s_mainItems[itemIdx], C_BLACK);
    }
  }

  // Scrollbar Vertikal di Sisi Kanan (x = 123)
  fbVLine(123, 16, 46, C_BLACK);
  int thumbH = 8;
  int thumbY = 16 + (s_mainMenuIdx * (46 - thumbH)) / (MAIN_MENU_COUNT - 1);
  fbFillRect(121, thumbY, 3, thumbH, C_BLACK);

  displayFlush();
  delay(20);
}

// -----------------------------------------------------------------------------
// 2. RENDER & LOGIKA SUBMENU 1: PILIH EKSPRESI (26 Mood Teks)
// -----------------------------------------------------------------------------
static void updateSubmenuEkspresi(uint32_t now) {
  if (s_isConfirming) {
    if (now - s_confirmStartMs >= 650) {
      s_isConfirming = false;
      menuClose();
      return;
    }
  } else {
    if (g_navPending) {
      NavEvent ev = g_pendingNav;
      g_navPending = false;

      if (ev == NAV_TAP) {
        s_moodIdx = (s_moodIdx + 1) % MOOD_COUNT;
        s_lastActivityMs = now;
        if (s_moodIdx >= s_moodTopIdx + 4) {
          s_moodTopIdx = s_moodIdx - 3;
        } else if (s_moodIdx < s_moodTopIdx) {
          s_moodTopIdx = 0;
        }
      } else if (ev == NAV_HOLD || ev == NAV_DOUBLE_TAP) {
        s_state = MENU_STATE_MAIN;
        s_lastActivityMs = now;
        return;
      }
    }

    if (g_touchPending) {
      g_touchPending = false;
      s_isConfirming   = true;
      s_confirmStartMs = now;
      s_isLocked       = true;
      s_lockedMood     = (Mood)s_moodIdx;
      Serial.printf("[menu] Terpilih: %s! Mengunci ke Home.\n", moodName(s_lockedMood));
    }

    if (now - s_lastActivityMs > 15000) {
      menuClose();
      return;
    }
  }

  displayClear();

  // Header Title Bar
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);
  int titleW = fbGetStringWidth("PILIH EKSPRESI");
  fbDrawString((OLED_W - titleW) / 2, 3, "PILIH EKSPRESI", C_WHITE);

  for (int i = 0; i < 4; ++i) {
    int itemIdx = s_moodTopIdx + i;
    if (itemIdx >= MOOD_COUNT) break;

    int y = 16 + i * 11;
    const char* mName = moodName((Mood)itemIdx);

    if (itemIdx == s_moodIdx) {
      fbFillRoundRect(4, y, 114, 11, 2, C_BLACK);
      fbDrawRightArrow(8, y + 3, C_WHITE);
      fbDrawString(18, y + 2, mName, C_WHITE);
    } else {
      fbDrawString(18, y + 2, mName, C_BLACK);
    }
  }

  // Scrollbar vertikal
  fbVLine(123, 16, 46, C_BLACK);
  int thumbH = 8;
  int thumbY = 16 + (s_moodIdx * (46 - thumbH)) / 25;
  fbFillRect(121, thumbY, 3, thumbH, C_BLACK);

  // Pop-up Feedback Konfirmasi
  if (s_isConfirming) {
    fbFillRoundRect(12, 18, 104, 28, 3, C_BLACK);
    fbRect(13, 19, 102, 26, C_WHITE);
    fbDrawCheckmark(18, 29, C_WHITE);

    char confBuf[24];
    snprintf(confBuf, sizeof(confBuf), "%s OK!", moodName(s_lockedMood));
    fbDrawString(32, 28, confBuf, C_WHITE);
  }

  displayFlush();
  delay(15);
}

// -----------------------------------------------------------------------------
// 3. RENDER & LOGIKA SUBMENU 5: RIWAYAT NOTIFIKASI (INBOX 5 PESAN)
// -----------------------------------------------------------------------------
static void updateSubmenuInbox(uint32_t now) {
  uint8_t count = bleGetInboxCount();

  // Konfirmasi feedback riwayat dibersihkan
  if (s_inboxClearedConf) {
    if (now - s_inboxConfStartMs >= 800) {
      s_inboxClearedConf = false;
      s_inboxMenuIdx = 0;
      s_inboxTopIdx  = 0;
    }
    displayClear();
    fbFillRect(0, 0, OLED_W, 13, C_BLACK);
    int tW = fbGetStringWidth("RIWAYAT NOTIF");
    fbDrawString((OLED_W - tW) / 2, 3, "RIWAYAT NOTIF", C_WHITE);

    fbFillRoundRect(14, 22, 100, 26, 2, C_BLACK);
    fbDrawCheckmark(20, 32, C_WHITE);
    fbDrawString(34, 30, "INBOX BERSIH!", C_WHITE);

    displayFlush();
    delay(20);
    return;
  }

  // ---------------------------------------------------------------------------
  // TAMPILAN DETAIL PESAN TERTENTU
  // ---------------------------------------------------------------------------
  if (s_inboxView == INBOX_VIEW_DETAIL) {
    if (g_touchPending || g_navPending) {
      g_touchPending = false;
      g_navPending   = false;
      s_inboxView    = INBOX_VIEW_LIST;
      s_lastActivityMs = now;
      return;
    }

    if (now - s_lastActivityMs > 25000) {
      s_inboxView = INBOX_VIEW_LIST;
      return;
    }

    const BleNotification* item = bleGetInboxItem(s_inboxSelectedIdx);
    if (!item || !item->valid) {
      s_inboxView = INBOX_VIEW_LIST;
      return;
    }

    displayClear();

    // 1. Header Bar
    fbFillRect(0, 0, OLED_W, 12, C_BLACK);
    char hdr[24];
    snprintf(hdr, sizeof(hdr), "[%s] %s", item->app, item->sender);
    fbDrawString(4, 2, hdr, C_WHITE);

    // 2. Garis Pemisah
    fbHLine(4, 14, 120, C_BLACK);

    // 3. Isi Teks Pesan (Word Wrap 4 baris rapi)
    char lines[4][24];
    wrapInboxBody(item->body, lines);
    for (int r = 0; r < 4; ++r) {
      if (lines[r][0] != '\0') {
        fbDrawString(4, 16 + r * 9, lines[r], C_BLACK);
      }
    }

    displayFlush();
    delay(20);
    return;
  }

  // ---------------------------------------------------------------------------
  // TAMPILAN DAFTAR PESAN (LIST)
  // ---------------------------------------------------------------------------
  // KASUS 1: JIKA INBOX MASIH KOSONG (BELUM ADA NOTIFIKASI DARI HP)
  if (count == 0) {
    if (g_touchPending || g_navPending) {
      g_touchPending = false;
      g_navPending   = false;
      s_state        = MENU_STATE_MAIN;
      s_lastActivityMs = now;
      return;
    }

    if (now - s_lastActivityMs > 20000) {
      menuClose();
      return;
    }

    displayClear();

    // Header Bar
    fbFillRect(0, 0, OLED_W, 13, C_BLACK);
    int tW = fbGetStringWidth("RIWAYAT NOTIF (0)");
    fbDrawString((OLED_W - tW) / 2, 3, "RIWAYAT NOTIF (0)", C_WHITE);

    // Pesan Informasi
    int w1 = fbGetStringWidth("Belum ada notifikasi");
    fbDrawString((OLED_W - w1) / 2, 21, "Belum ada notifikasi", C_BLACK);

    int w2 = fbGetStringWidth("Kirim dari aplikasi HP");
    fbDrawString((OLED_W - w2) / 2, 32, "Kirim dari aplikasi HP", C_BLACK);

    // Tombol Bersih Kembali
    fbFillRoundRect(8, 48, 112, 12, 2, C_BLACK);
    int bW = fbGetStringWidth("KEMBALI KE MENU");
    fbDrawString((OLED_W - bW) / 2, 50, "KEMBALI KE MENU", C_WHITE);

    displayFlush();
    delay(20);
    return;
  }

  // KASUS 2: JIKA ADA NOTIFIKASI TERSIMPAN (count > 0)
  int totalItems = count + 2;

  if (g_navPending) {
    NavEvent ev = g_pendingNav;
    g_navPending = false;

    if (ev == NAV_TAP) {
      s_inboxMenuIdx = (s_inboxMenuIdx + 1) % totalItems;
      s_lastActivityMs = now;
      if (s_inboxMenuIdx >= s_inboxTopIdx + 4) {
        s_inboxTopIdx = s_inboxMenuIdx - 3;
      } else if (s_inboxMenuIdx < s_inboxTopIdx) {
        s_inboxTopIdx = s_inboxMenuIdx;
      }
    } else if (ev == NAV_HOLD || ev == NAV_DOUBLE_TAP) {
      s_state = MENU_STATE_MAIN;
      s_lastActivityMs = now;
      return;
    }
  }

  if (g_touchPending) {
    g_touchPending = false;
    s_lastActivityMs = now;

    if (s_inboxMenuIdx < count) {
      // Buka Detail Pesan
      s_inboxSelectedIdx = s_inboxMenuIdx;
      s_inboxView        = INBOX_VIEW_DETAIL;
      return;
    } else if (s_inboxMenuIdx == count) {
      // Kosongkan Riwayat
      bleClearInbox();
      s_inboxClearedConf = true;
      s_inboxConfStartMs = now;
      return;
    } else {
      // Kembali ke Menu Utama
      s_state = MENU_STATE_MAIN;
      return;
    }
  }

  if (now - s_lastActivityMs > 20000) {
    menuClose();
    return;
  }

  displayClear();

  // Header Title Bar
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);
  char titleBuf[24];
  snprintf(titleBuf, sizeof(titleBuf), "RIWAYAT NOTIF (%d)", count);
  int tW = fbGetStringWidth(titleBuf);
  fbDrawString((OLED_W - tW) / 2, 3, titleBuf, C_WHITE);

  // Render Item dalam Viewport
  for (int i = 0; i < 4; ++i) {
    int itemIdx = s_inboxTopIdx + i;
    if (itemIdx >= totalItems) break;

    int y = 16 + i * 11;
    char label[28];

    if (itemIdx < count) {
      const BleNotification* it = bleGetInboxItem(itemIdx);
      if (it && it->valid) {
        snprintf(label, sizeof(label), "%d.[%s] %s", itemIdx + 1, it->app, it->sender);
      } else {
        snprintf(label, sizeof(label), "%d. Pesan Kosong", itemIdx + 1);
      }
    } else if (itemIdx == count) {
      snprintf(label, sizeof(label), "KOSONGKAN INBOX");
    } else {
      snprintf(label, sizeof(label), "KEMBALI KE MENU");
    }

    if (itemIdx == s_inboxMenuIdx) {
      fbFillRoundRect(4, y, 114, 11, 2, C_BLACK);
      fbDrawRightArrow(8, y + 3, C_WHITE);
      fbDrawString(18, y + 2, label, C_WHITE);
    } else {
      fbDrawString(18, y + 2, label, C_BLACK);
    }
  }

  // Scrollbar di sisi kanan
  if (totalItems > 4) {
    fbVLine(123, 16, 46, C_BLACK);
    int thumbH = 8;
    int thumbY = 16 + (s_inboxMenuIdx * (46 - thumbH)) / (totalItems - 1);
    fbFillRect(121, thumbY, 3, thumbH, C_BLACK);
  }

  displayFlush();
  delay(20);
}

// -----------------------------------------------------------------------------
// 4. RENDER & LOGIKA SUBMENU 7: PENGATURAN BMO
// -----------------------------------------------------------------------------
static void updateSubmenuSettings(uint32_t now) {
  static const int SETTINGS_COUNT = 4;

  if (g_navPending) {
    NavEvent ev = g_pendingNav;
    g_navPending = false;

    if (ev == NAV_TAP) {
      s_settingsIdx = (s_settingsIdx + 1) % SETTINGS_COUNT;
      s_lastActivityMs = now;
    } else if (ev == NAV_HOLD || ev == NAV_DOUBLE_TAP) {
      s_state = MENU_STATE_MAIN;
      s_lastActivityMs = now;
      return;
    }
  }

  if (g_touchPending) {
    g_touchPending = false;
    s_lastActivityMs = now;

    if (s_settingsIdx == 0) {
      // 1. Ganti Kontras Layar
      s_contrastLevel = (s_contrastLevel + 1) % 3;
      uint8_t cVal = (s_contrastLevel == 0) ? 35 : ((s_contrastLevel == 1) ? 128 : 255);
      displaySetContrast(cVal);

      // Simpan ke Flash
      s_cfgPrefs.begin("bmo_cfg", false);
      s_cfgPrefs.putUChar("contrast", s_contrastLevel);
      s_cfgPrefs.end();
      Serial.printf("[Settings] Kontras diubah ke level: %d (%d)\n", s_contrastLevel, cVal);
    } else if (s_settingsIdx == 1) {
      // 2. Putar Layar (Flip 180°)
      s_screenFlipped = !s_screenFlipped;
      displaySetFlip(s_screenFlipped);

      // Simpan ke Flash
      s_cfgPrefs.begin("bmo_cfg", false);
      s_cfgPrefs.putBool("flip", s_screenFlipped);
      s_cfgPrefs.end();
      Serial.printf("[Settings] Putar layar: %s\n", s_screenFlipped ? "180 Derajat" : "Normal 0");
    } else if (s_settingsIdx == 2) {
      // 3. Ganti Mode Kepribadian
      s_mochiEnabled = !s_mochiEnabled;
      s_cfgPrefs.begin("bmo_cfg", false);
      s_cfgPrefs.putBool("mochi", s_mochiEnabled);
      s_cfgPrefs.end();
      Serial.printf("[Settings] Personality Engine: %s\n", s_mochiEnabled ? "Mochi Dinamis" : "Rutin 26 Mood");
    } else {
      // 4. Kembali ke Menu Utama
      s_state = MENU_STATE_MAIN;
      return;
    }
  }

  if (now - s_lastActivityMs > 20000) {
    menuClose();
    return;
  }

  displayClear();

  // Header Title Bar
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);
  int tW = fbGetStringWidth("PENGATURAN BMO");
  fbDrawString((OLED_W - tW) / 2, 3, "PENGATURAN BMO", C_WHITE);

  // Label opsi-opsi
  char opt0[24];
  const char* cLabels[] = { "RENDAH", "SEDANG", "MAX" };
  snprintf(opt0, sizeof(opt0), "KONTRAS: [%s]", cLabels[s_contrastLevel]);

  char opt1[24];
  snprintf(opt1, sizeof(opt1), "PUTAR LAYAR: [%s]", s_screenFlipped ? "180^" : "0^");

  char opt2[24];
  snprintf(opt2, sizeof(opt2), "IDLE: [%s]", s_mochiEnabled ? "MOCHI" : "RUTIN");

  const char* optLabels[4] = { opt0, opt1, opt2, "KEMBALI KE MENU" };

  for (int i = 0; i < 4; ++i) {
    int y = 16 + i * 11;
    if (i == s_settingsIdx) {
      fbFillRoundRect(4, y, 120, 11, 2, C_BLACK);
      fbDrawRightArrow(8, y + 3, C_WHITE);
      fbDrawString(18, y + 2, optLabels[i], C_WHITE);
    } else {
      fbDrawString(18, y + 2, optLabels[i], C_BLACK);
    }
  }

  fbHLine(6, 61, 116, C_BLACK);

  displayFlush();
  delay(20);
}

// -----------------------------------------------------------------------------
// UPDATE LOOP UTAMA SISTEM MENU
// -----------------------------------------------------------------------------
void menuUpdate() {
  uint32_t now = millis();
  pollTouch();

  switch (s_state) {
    case MENU_STATE_MAIN:
      updateMainMenu(now);
      break;

    case MENU_STATE_SUB_EKSPRESI:
      updateSubmenuEkspresi(now);
      break;

    case MENU_STATE_SUB_TIME:
      timeMenuUpdate();
      if (!timeMenuIsActive()) {
        s_state = MENU_STATE_MAIN;
        s_lastActivityMs = now;
      }
      break;

    case MENU_STATE_SUB_TAMAGOTCHI:
      tamagotchiUpdate();
      if (!tamagotchiIsActive()) {
        s_state = MENU_STATE_MAIN;
        s_lastActivityMs = now;
      }
      break;

    case MENU_STATE_SUB_GAME:
      gameUpdate();
      if (!gameIsActive()) {
        s_state = MENU_STATE_MAIN;
        s_lastActivityMs = now;
      }
      break;

    case MENU_STATE_SUB_INBOX:
      updateSubmenuInbox(now);
      break;

    case MENU_STATE_SUB_BLE:
      bleMenuUpdate();
      break;

    case MENU_STATE_SUB_SETTINGS:
      updateSubmenuSettings(now);
      break;

    default:
      break;
  }
}
