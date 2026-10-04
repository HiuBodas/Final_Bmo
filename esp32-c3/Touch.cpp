// =============================================================================
// Touch.cpp - Implementasi Pembacaan Debounced & Klasifikasi Gestur Sentuh
// Sensor 1: TTP223 pada GPIO 20 (Wajah BMO & Konfirmasi OK)
// Sensor 2: TTP223 pada GPIO 21 (Navigasi Menu Pindah Ekspresi)
// =============================================================================
#include "Touch.h"

#if defined(ESP32)
#include "driver/gpio.h"
#endif

// Status interupsi Sensor 1 (GP20)
volatile bool      g_touchPending = false;
volatile TouchKind g_pendingKind  = TOUCH_NONE;

// Status interupsi Sensor 2 (GP21)
volatile bool      g_navPending   = false;
volatile NavEvent  g_pendingNav   = NAV_NONE;

// Debounce 15ms: responsif terhadap ketukan kilat (poke) tanpa false-trigger
static constexpr uint32_t kDebounceMs = 15;

// Status internal Sensor GP20
static uint32_t s_recentPressMs[8] = {0};
static uint8_t  s_pressIdx         = 0;
static int      s_idleLevel20      = LOW;

// Status internal Sensor GP21
static int      s_idleLevel21      = LOW;
static uint32_t s_lastNavTapMs     = 0;

void touchInit() {
#if defined(ESP32)
  // Lepaskan pin GPIO 20 & GPIO 21 dari UART0 peripheral bawaan
  gpio_reset_pin((gpio_num_t)PIN_TOUCH);
  gpio_reset_pin((gpio_num_t)PIN_TOUCH_NAV);
#endif

  // Set kedua pin sebagai INPUT murni (TTP223 memiliki driver output CMOS aktif)
  pinMode(PIN_TOUCH, INPUT);
  pinMode(PIN_TOUCH_NAV, INPUT);
  delay(20);

  // Auto-deteksi polaritas idle untuk GP20
  int high20 = 0;
  for (int i = 0; i < 16; ++i) {
    if (digitalRead(PIN_TOUCH) == HIGH) high20++;
    delay(2);
  }
  s_idleLevel20 = (high20 >= 9) ? HIGH : LOW;

  // Auto-deteksi polaritas idle untuk GP21
  int high21 = 0;
  for (int i = 0; i < 16; ++i) {
    if (digitalRead(PIN_TOUCH_NAV) == HIGH) high21++;
    delay(2);
  }
  s_idleLevel21 = (high21 >= 9) ? HIGH : LOW;

  Serial.printf("[touch] Sensor 1 ready: IdleLevel=%d (Mode: Active-%s)\n",
                s_idleLevel20, (s_idleLevel20 == LOW) ? "HIGH" : "LOW");
  Serial.printf("[touch] Sensor 2 ready: IdleLevel=%d (Mode: Active-%s)\n",
                s_idleLevel21, (s_idleLevel21 == LOW) ? "HIGH" : "LOW");
}

static inline bool touchRaw20() {
  return digitalRead(PIN_TOUCH) != s_idleLevel20;
}

static inline bool touchRaw21() {
  return digitalRead(PIN_TOUCH_NAV) != s_idleLevel21;
}

bool touchIsDown() {
  static bool stable = false;
  static bool lastRaw = false;
  static uint32_t lastChange = 0;

  bool raw = touchRaw20();
  if (raw != lastRaw) {
    lastRaw = raw;
    lastChange = millis();
  } else if (raw != stable && (millis() - lastChange) >= kDebounceMs) {
    stable = raw;
  }
  return stable;
}

bool touchNavIsDown() {
  static bool stable = false;
  static bool lastRaw = false;
  static uint32_t lastChange = 0;

  bool raw = touchRaw21();
  if (raw != lastRaw) {
    lastRaw = raw;
    lastChange = millis();
  } else if (raw != stable && (millis() - lastChange) >= kDebounceMs) {
    stable = raw;
  }
  return stable;
}

static int recentPressesWithin(uint32_t windowMs) {
  uint32_t now = millis();
  int count = 0;
  for (size_t i = 0; i < sizeof(s_recentPressMs) / sizeof(s_recentPressMs[0]); ++i) {
    if (s_recentPressMs[i] != 0 && (now - s_recentPressMs[i]) <= windowMs) {
      count++;
    }
  }
  return count;
}

void pollTouch() {
  uint32_t now = millis();

  // ---------------------------------------------------------------------------
  // 1. Polling Sensor 1 (Aksi / Wajah & Konfirmasi OK)
  // ---------------------------------------------------------------------------
  static bool wasDown20 = false;
  static uint32_t pressStart20 = 0;
  static bool longHoldFired20 = false;

  bool isDown20 = touchIsDown();

  if (isDown20 && !wasDown20) {
    pressStart20 = now;
    s_recentPressMs[s_pressIdx % 8] = pressStart20;
    s_pressIdx++;
    longHoldFired20 = false;
  } else if (isDown20 && !longHoldFired20) {
    if (now - pressStart20 >= 1500) {
      longHoldFired20 = true;
      Serial.println("[touch] Sensor 1: LONG HOLD / LOVE");
      g_pendingKind  = TOUCH_LONG_HOLD;
      g_touchPending = true;
    }
  } else if (!isDown20 && wasDown20) {
    if (!longHoldFired20) {
      uint32_t held = now - pressStart20;
      int taps = recentPressesWithin(900);
      TouchKind kind;

      if (taps >= 3 && held < 350) {
        kind = TOUCH_TICKLE;
      } else if (held < 250) {
        kind = TOUCH_QUICK_POKE;
      } else {
        kind = TOUCH_HOLD;
      }

      Serial.printf("[touch] Sensor 1: %s (%lu ms)\n", touchKindName(kind), (unsigned long)held);
      g_pendingKind  = kind;
      g_touchPending = true;
    }
  }
  wasDown20 = isDown20;

  // ---------------------------------------------------------------------------
  // 2. Polling Sensor 2 (Navigasi Menu)
  // ---------------------------------------------------------------------------
  static bool wasDown21 = false;
  static uint32_t pressStart21 = 0;
  static bool navHoldFired = false;

  bool isDown21 = touchNavIsDown();

  if (isDown21 && !wasDown21) {
    pressStart21 = now;
    navHoldFired = false;
  } else if (isDown21 && !navHoldFired) {
    if (now - pressStart21 >= 850) {
      navHoldFired = true;
      Serial.println("[nav] Sensor 2: HOLD");
      g_pendingNav = NAV_HOLD;
      g_navPending = true;
    }
  } else if (!isDown21 && wasDown21) {
    if (!navHoldFired) {
      uint32_t held = now - pressStart21;
      if (held < 450) {
        // Cek double tap
        if (s_lastNavTapMs != 0 && (now - s_lastNavTapMs) < 350) {
          Serial.println("[nav] Sensor 2: DOUBLE TAP");
          g_pendingNav = NAV_DOUBLE_TAP;
          g_navPending = true;
          s_lastNavTapMs = 0;
        } else {
          Serial.println("[nav] Sensor 2: TAP");
          g_pendingNav = NAV_TAP;
          g_navPending = true;
          s_lastNavTapMs = now;
        }
      }
    }
  }
  wasDown21 = isDown21;
}

const char* touchKindName(TouchKind k) {
  switch (k) {
    case TOUCH_QUICK_POKE: return "QUICK POKE";
    case TOUCH_HOLD:       return "HOLD / PET";
    case TOUCH_LONG_HOLD:  return "LONG HOLD / LOVE";
    case TOUCH_TICKLE:     return "TICKLE";
    default:               return "NONE";
  }
}

const char* navEventName(NavEvent e) {
  switch (e) {
    case NAV_TAP:        return "NAV TAP";
    case NAV_HOLD:       return "NAV HOLD";
    case NAV_DOUBLE_TAP: return "NAV DOUBLE TAP";
    default:             return "NAV NONE";
  }
}

