// =============================================================================
// BleManager.cpp - Implementasi Modul BLE & Notifikasi Real-Time BMO
// 100% Vektor Prosedural Tanpa Bitmaps (OLED SSD1306 128x64)
// =============================================================================
#include "BleManager.h"
#include "Display.h"
#include "Touch.h"
#include "Menu.h"
#include "Moods.h"
#include "WifiManager.h"
#include "WeatherManager.h"
#include "SpotifyWidget.h"
#include "DoodleWidget.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// --- KONFIGURASI BLE (NORDIC UART SERVICE) ---
#define BLE_DEVICE_NAME        "BMO"
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Variabel Internal BLE
static BLEServer*         s_pServer           = nullptr;
static BLECharacteristic* s_pTxCharacteristic = nullptr;
static BLECharacteristic* s_pRxCharacteristic = nullptr;
static BLEAdvertising*    s_pAdv              = nullptr;

static bool        s_bleInitialized = false;
static bool        s_bleActive      = false;
static bool        s_deviceConnected= false;
static BleTargetOS s_currentTargetOS= BLE_OS_NONE;

// Variabel Notifikasi Real-time
static const uint32_t   NOTIF_DURATION_MS = 7000; // Tampil 7 detik jika tidak disentuh
static bool             s_notifActive     = false;
static uint32_t         s_notifStartMs    = 0;
static BleNotification  s_currentNotif;

// Riwayat Notifikasi Masuk (Inbox 5 Pesan Terakhir)
#define BLE_INBOX_CAPACITY 5
static BleNotification  s_inbox[BLE_INBOX_CAPACITY];
static uint8_t          s_inboxCount      = 0;

// Status Navigasi Submenu & Koneksi BLE
enum BleSubState : uint8_t {
  BLE_SUB_MENU = 0,
  BLE_SUB_CONNECTING,
  BLE_SUB_CONNECTED_CONFIRM
};
static BleSubState      s_bleSubState     = BLE_SUB_MENU;
static int              s_bleMenuIdx      = 0;
static uint32_t         s_connectStartMs  = 0;
static uint32_t         s_connectSuccessMs= 0;
static uint32_t         s_lastMenuActMs   = 0;

// -----------------------------------------------------------------------------
// Callback Koneksi BLE Server
// -----------------------------------------------------------------------------
class BmoServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    s_deviceConnected = true;
    Serial.println(F("[BLE] Smartphone Terhubung ke BMO!"));
    if (s_bleSubState == BLE_SUB_CONNECTING) {
      s_bleSubState = BLE_SUB_CONNECTED_CONFIRM;
      s_connectSuccessMs = millis();
    }
  }

  void onDisconnect(BLEServer* pServer) override {
    s_deviceConnected = false;
    Serial.println(F("[BLE] Smartphone Terputus!"));
    if (s_bleActive && pServer != nullptr) {
      pServer->startAdvertising();
      Serial.println(F("[BLE] Advertising ulang... Siap menerima koneksi baru."));
    }
  }
};

// -----------------------------------------------------------------------------
// Callback Penerimaan Data Karakteristik RX (HP -> ESP32)
// -----------------------------------------------------------------------------
class BmoReceiveCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String rxValue = pCharacteristic->getValue().c_str();
    rxValue.trim();

    // Fallback jika getValue() kosong
    if (rxValue.length() == 0) {
      size_t len = pCharacteristic->getLength();
      uint8_t* data = pCharacteristic->getData();
      if (len > 0 && data != nullptr) {
        for (size_t i = 0; i < len; i++) {
          rxValue += (char)data[i];
        }
        rxValue.trim();
      }
    }

    Serial.print(F("[BLE RX] Pesan Masuk: "));
    Serial.println(rxValue);

    if (rxValue.length() > 0) {
      parseIncomingBleMessage(rxValue);
      bleSend("ACK_OK");
    }
  }
};

// -----------------------------------------------------------------------------
// Inisialisasi Stack BLE
// -----------------------------------------------------------------------------
static void initBleStackInternal() {
  if (s_bleInitialized) return;

  BLEDevice::init(BLE_DEVICE_NAME);
  BLEDevice::setMTU(517);

  s_pServer = BLEDevice::createServer();
  s_pServer->setCallbacks(new BmoServerCallbacks());

  BLEService* pService = s_pServer->createService(SERVICE_UUID);

  // Karakteristik TX (ESP32 -> Smartphone)
  s_pTxCharacteristic = pService->createCharacteristic(
                          CHARACTERISTIC_UUID_TX,
                          BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
                        );
  s_pTxCharacteristic->addDescriptor(new BLE2902());

  // Karakteristik RX (Smartphone -> ESP32)
  s_pRxCharacteristic = pService->createCharacteristic(
                          CHARACTERISTIC_UUID_RX,
                          BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
                        );
  s_pRxCharacteristic->setCallbacks(new BmoReceiveCallbacks());

  pService->start();

  // Konfigurasi Advertising
  s_pAdv = BLEDevice::getAdvertising();
  s_pAdv->addServiceUUID(SERVICE_UUID);
  s_pAdv->setScanResponse(true);
  s_pAdv->setMinPreferred(0x06);
  s_pAdv->setMinPreferred(0x12);

  BLEAdvertisementData advData;
  advData.setFlags(0x06); // General Discoverable + BR/EDR Not Supported
  advData.setName(BLE_DEVICE_NAME);
  advData.setCompleteServices(BLEUUID(SERVICE_UUID));
  s_pAdv->setAdvertisementData(advData);

  BLEAdvertisementData scanData;
  scanData.setName(BLE_DEVICE_NAME);
  s_pAdv->setScanResponseData(scanData);

  s_bleInitialized = true;
  Serial.println(F("[BLE] Stack BLE Nordic UART Service siap."));
}

void bleInit() {
  s_bleActive       = false;
  s_deviceConnected = false;
  s_currentTargetOS = BLE_OS_NONE;
  s_notifActive     = false;
  s_bleSubState     = BLE_SUB_MENU;
  s_bleMenuIdx      = 0;
}

// Dipanggil setiap kali submenu BLE dibuka dari Menu Utama.
// Mereset timer inaktivitas agar menu tidak langsung tertutup (timer lama/basi).
void bleMenuOpen() {
  s_bleSubState   = BLE_SUB_MENU;
  s_lastMenuActMs = millis();
  g_touchPending  = false;
  g_navPending    = false;
}

void bleStart(BleTargetOS target) {
  initBleStackInternal();

  s_currentTargetOS = target;
  s_bleActive       = true;

  if (s_pAdv != nullptr) {
    s_pAdv->start();
  } else {
    BLEDevice::startAdvertising();
  }

  Serial.printf("[BLE] Aktif (Target: %s) - Nama: %s\n",
                target == BLE_OS_ANDROID ? "Android" : "iOS (iPhone)",
                BLE_DEVICE_NAME);
}

void bleStop() {
  if (s_bleActive) {
    s_bleActive = false;

    if (s_pAdv != nullptr) {
      s_pAdv->stop();
    }

    if (s_deviceConnected && s_pServer != nullptr) {
      s_pServer->disconnect(s_pServer->getConnId());
    }

    s_deviceConnected = false;
    s_currentTargetOS = BLE_OS_NONE;
    Serial.println(F("[BLE] Bluetooth Dinonaktifkan (OFF)."));
  }
}

bool bleIsActive() {
  return s_bleActive;
}

bool bleIsConnected() {
  return s_deviceConnected;
}

BleTargetOS bleGetTargetOS() {
  return s_currentTargetOS;
}

const char* bleGetDeviceName() {
  return BLE_DEVICE_NAME;
}

void bleSend(const String& msg) {
  if (s_deviceConnected && s_pTxCharacteristic != nullptr) {
    s_pTxCharacteristic->setValue(msg.c_str());
    s_pTxCharacteristic->notify();
  }
}

// Triggers Ekspresi & Reaksi Sentuhan dari APK (Thread-safe antar FreeRTOS Core)
static volatile int16_t s_pendingBleMood = -1;
static volatile int16_t s_pendingBleReaction = -1;

bool bleHasPendingAction() {
  return (s_pendingBleMood >= 0 || s_pendingBleReaction >= 0 || s_notifActive);
}

bool bleHasPendingMood(Mood &m) {
  if (s_pendingBleMood >= 0) {
    m = (Mood)s_pendingBleMood;
    s_pendingBleMood = -1;
    return true;
  }
  return false;
}

bool bleHasPendingReaction(TouchKind &k) {
  if (s_pendingBleReaction >= 0) {
    k = (TouchKind)s_pendingBleReaction;
    s_pendingBleReaction = -1;
    return true;
  }
  return false;
}

void bleTriggerMood(Mood m) {
  s_pendingBleMood = (int16_t)m;
}

void blePushNotification(const char* app, const char* sender, const char* body) {
  snprintf(s_currentNotif.app, sizeof(s_currentNotif.app), "%s", app ? app : "MEMO");
  snprintf(s_currentNotif.sender, sizeof(s_currentNotif.sender), "%s", sender ? sender : "PC");
  snprintf(s_currentNotif.body, sizeof(s_currentNotif.body), "%s", body ? body : "");
  s_currentNotif.receivedMs = millis();
  s_currentNotif.valid = true;

  for (int i = BLE_INBOX_CAPACITY - 1; i > 0; --i) {
    s_inbox[i] = s_inbox[i - 1];
  }
  s_inbox[0] = s_currentNotif;
  if (s_inboxCount < BLE_INBOX_CAPACITY) s_inboxCount++;

  g_touchPending = false;
  g_navPending = false;
  s_notifStartMs = millis();
  s_notifActive = true;
}

// Membersihkan karakter non-ASCII, byte emoji, atau kode kontrol aneh agar font OLED bersih
static String cleanBleText(const String& in) {
  String out = "";
  out.reserve(in.length());
  for (size_t i = 0; i < in.length(); ++i) {
    char c = in[i];
    if (c >= 32 && c <= 126) {
      out += c;
    } else if (c == '\n' || c == '\r') {
      out += ' ';
    }
  }
  out.trim();
  return out;
}

// -----------------------------------------------------------------------------
// Parser Notifikasi Masuk (Mendukung format kurung siku [APP], format pipa |, & teks polos)
// -----------------------------------------------------------------------------
void parseIncomingBleMessage(const String& raw) {
  String s = cleanBleText(raw);
  if (s.length() == 0) return;

  String appName = "";
  String sender  = "";
  String body    = "";

  // 1. FORMAT KURUNG SIKU DARI APK ANDROID: [APP] Sender: Message
  if (s.startsWith("[") && s.indexOf(']') != -1) {
    int closeIdx = s.indexOf(']');
    String tag = s.substring(1, closeIdx);
    tag.toUpperCase();
    tag.trim();
    String content = s.substring(closeIdx + 1);
    content.trim();

    // A. Perintah Khusus Simulator BMO (Ekspresi Wajah): [BMO] MOOD:HAPPY
    if (tag == "BMO") {
      if (content.startsWith("MOOD:")) {
        String mName = content.substring(5);
        mName.toUpperCase();
        mName.trim();
        Serial.printf("[BMO BLE] Trigger Mood via APK: %s\n", mName.c_str());
        if (mName == "HAPPY") s_pendingBleMood = (int16_t)MOOD_HAPPY;
        else if (mName == "IDLE") s_pendingBleMood = (int16_t)MOOD_IDLE;
        else if (mName == "COOL") s_pendingBleMood = (int16_t)MOOD_COOL;
        else if (mName == "LOVE") s_pendingBleMood = (int16_t)MOOD_LOVE;
        else if (mName == "WINK") s_pendingBleMood = (int16_t)MOOD_WINK;
        else if (mName == "LAUGH") s_pendingBleMood = (int16_t)MOOD_LAUGH;
        else if (mName == "TALK") s_pendingBleMood = (int16_t)MOOD_TALK;
        else if (mName == "LISTEN") s_pendingBleMood = (int16_t)MOOD_LISTEN;
        else if (mName == "FOCUSED") s_pendingBleMood = (int16_t)MOOD_FOCUSED;
        else if (mName == "THINKING") s_pendingBleMood = (int16_t)MOOD_THINKING;
        else if (mName == "EXCITED") s_pendingBleMood = (int16_t)MOOD_EXCITED;
        else if (mName == "ANGRY") s_pendingBleMood = (int16_t)MOOD_ANGRY;
        else if (mName == "SAD") s_pendingBleMood = (int16_t)MOOD_SAD;
        else if (mName == "SURPRISE") s_pendingBleMood = (int16_t)MOOD_SURPRISE;
        else if (mName == "SLEEPY") s_pendingBleMood = (int16_t)MOOD_SLEEPY;
        else if (mName == "HUNGRY") s_pendingBleMood = (int16_t)MOOD_HUNGRY;
        else if (mName == "COLD") s_pendingBleMood = (int16_t)MOOD_COLD;
        else if (mName == "HOT") s_pendingBleMood = (int16_t)MOOD_HOT;
        else if (mName == "SICK") s_pendingBleMood = (int16_t)MOOD_SICK;
        else if (mName == "DIZZY") s_pendingBleMood = (int16_t)MOOD_DIZZY;
        else if (mName == "GLITCH") s_pendingBleMood = (int16_t)MOOD_GLITCH;
        else if (mName == "CONFUSED") s_pendingBleMood = (int16_t)MOOD_CONFUSED;
        else if (mName == "BORED") s_pendingBleMood = (int16_t)MOOD_BORED;
        else if (mName == "SCARED") s_pendingBleMood = (int16_t)MOOD_SCARED;
        else if (mName == "WAKE") s_pendingBleMood = (int16_t)MOOD_WAKE;
        else if (mName == "BLINK") s_pendingBleMood = (int16_t)MOOD_BLINK;
        else s_pendingBleMood = (int16_t)MOOD_HAPPY;
        return;
      } else if (content.startsWith("TOUCH:")) {
        String tName = content.substring(6);
        tName.toUpperCase();
        tName.trim();
        Serial.printf("[BMO BLE] Trigger Touch via APK: %s\n", tName.c_str());
        if (tName == "TICKLE") s_pendingBleReaction = (int16_t)TOUCH_TICKLE;
        else if (tName == "POKE") s_pendingBleReaction = (int16_t)TOUCH_QUICK_POKE;
        else if (tName == "HOLD") s_pendingBleReaction = (int16_t)TOUCH_HOLD;
        else if (tName == "LONG_HOLD" || tName == "HUG") s_pendingBleReaction = (int16_t)TOUCH_LONG_HOLD;
        else s_pendingBleReaction = (int16_t)TOUCH_QUICK_POKE;
        return;
      } else if (content.startsWith("NAME:")) {
        String newName = content.substring(5);
        newName.trim();
        displaySetCustomName(newName);
        bleSend("[NAME] OK:" + displayGetCustomName());
        return;
      }
    }
    // B. Perintah Custom Nama BMO: [NAME] NamaBaru atau [NAME] GET
    else if (tag == "NAME") {
      String newName = content;
      newName.trim();
      if (newName.equalsIgnoreCase("GET")) {
        bleSend("[NAME] " + displayGetCustomName());
      } else {
        displaySetCustomName(newName);
        bleSend("[NAME] OK:" + displayGetCustomName());
      }
      return;
    }
    // C. Perintah Sinkronisasi Jam: [TIME] HH:mm:ss|dd/MM/yyyy|Day|100%
    else if (tag == "TIME") {
      appName = "JAM & WAKTU";
      sender  = "SINKRON HP";
      body    = content;
    }
    // C. Perintah Pengaturan Wi-Fi: [WIFI] SSID:PASS|URL, [WIFI] STATUS, [WIFI] DISCONNECT
    else if (tag == "WIFI") {
      if (content.equalsIgnoreCase("STATUS")) {
        wifiSendBleStatus();
        return;
      }
      if (content.equalsIgnoreCase("DISCONNECT")) {
        wifiDisconnect();
        bleSend("[WIFI] DISCONNECTED");
        return;
      }

      int colonIdx = content.indexOf(':');
      if (colonIdx != -1) {
        String ssid = content.substring(0, colonIdx);
        String rem  = content.substring(colonIdx + 1);
        String pass = "";
        String url  = "";

        int pipeIdx = rem.indexOf('|');
        if (pipeIdx != -1) {
          pass = rem.substring(0, pipeIdx);
          url  = rem.substring(pipeIdx + 1);
        } else {
          pass = rem;
        }

        ssid.trim();
        pass.trim();
        url.trim();

        if (ssid.length() > 0) {
          wifiSaveCredentials(ssid, pass, url);
          bleSend("[WIFI] CONNECTING");
          appName = "WI-FI";
          sender  = ssid;
          body    = "Menghubungkan...";
        } else {
          return;
        }
      } else {
        appName = "WI-FI";
        sender  = "INFO";
        body    = content;
      }
    }
    // D. Pengaturan Kota Cuaca: [WEATHER] CITY:Jakarta atau [WEATHER] Jakarta
    else if (tag == "WEATHER") {
      String city = content;
      if (city.startsWith("CITY:")) city = city.substring(5);
      city.trim();
      if (city.length() > 0) {
        weatherSetCity(city);
        weatherUpdate();
        bleSend("[WEATHER] OK:" + city);
        appName = "CUACA";
        sender  = "KOTA";
        body    = city;
      }
    }
    // E. Pengaturan Lagu Spotify: [SPOTIFY] Judul|Artis
    else if (tag == "SPOTIFY") {
      String title = content;
      String artist = "Musik";
      int pipeIdx = content.indexOf('|');
      if (pipeIdx != -1) {
        title  = content.substring(0, pipeIdx);
        artist = content.substring(pipeIdx + 1);
      }
      title.trim();
      artist.trim();
      if (title.length() > 0) {
        spotifySetTrack(title, artist);
        spotifyOpen();
        bleSend("[SPOTIFY] OK");
        return;
      }
    }
    // G. Sinkronisasi Lirik Real-Time: [LYRIC] BarisAktif|BarisBerikutnya
    else if (tag == "LYRIC") {
      String cur = content;
      String nxt = "";
      int pipeIdx = content.indexOf('|');
      if (pipeIdx != -1) {
        cur = content.substring(0, pipeIdx);
        nxt = content.substring(pipeIdx + 1);
      }
      cur.trim();
      nxt.trim();
      spotifySetLyricsLine(cur, nxt);
      bleSend("[LYRIC] OK");
      return;
    }
    // H. Permintaan Fetch Lirik Mandiri via Wi-Fi: [LYRIC_FETCH] Judul|Artis
    else if (tag == "LYRIC_FETCH") {
      String title = content;
      String artist = "";
      int pipeIdx = content.indexOf('|');
      if (pipeIdx != -1) {
        title  = content.substring(0, pipeIdx);
        artist = content.substring(pipeIdx + 1);
      }
      title.trim();
      artist.trim();
      spotifySetTrack(title, artist);
      spotifyOpen();
      spotifyFetchLyrics();
      bleSend("[LYRIC_FETCH] OK");
      return;
    }
    // I. Perintah Kanvas Gambar Langsung: [DRAW]
    else if (tag == "DRAW") {
      if (content.startsWith("CLEAR")) {
        bool whiteBg = true;
        if (content.indexOf(":0") != -1) whiteBg = false;
        doodleClear(whiteBg);
        doodleOpen();
        bleSend("[DRAW] OK");
        return;
      } else if (content.startsWith("EXIT")) {
        doodleClose();
        bleSend("[DRAW] EXIT");
        return;
      } else if (content.startsWith("INVERT:")) {
        bool inv = content.substring(7).toInt() == 1;
        doodleSetInvert(inv);
        doodleOpen();
        bleSend("[DRAW] OK");
        return;
      } else if (content.startsWith("L:")) {
        // [DRAW] L:x0,y0,x1,y1
        int c1 = content.indexOf(',', 2);
        int c2 = (c1 != -1) ? content.indexOf(',', c1 + 1) : -1;
        int c3 = (c2 != -1) ? content.indexOf(',', c2 + 1) : -1;
        if (c1 != -1 && c2 != -1 && c3 != -1) {
          int x0 = content.substring(2, c1).toInt();
          int y0 = content.substring(c1 + 1, c2).toInt();
          int x1 = content.substring(c2 + 1, c3).toInt();
          int y1 = content.substring(c3 + 1).toInt();
          doodleDrawLine(x0, y0, x1, y1, C_BLACK);
          doodleOpen();
        }
        return;
      } else if (content.startsWith("P:")) {
        // [DRAW] P:pageIdx:HEX_DATA
        int c1 = content.indexOf(':', 2);
        if (c1 != -1) {
          int pageIdx = content.substring(2, c1).toInt();
          String hex = content.substring(c1 + 1);
          hex.trim();
          uint8_t pageData[128];
          int byteCount = 0;
          for (unsigned int i = 0; i + 1 < hex.length() && byteCount < 128; i += 2) {
            char ch1 = hex[i];
            char ch2 = hex[i + 1];
            auto h2v = [](char c) -> uint8_t {
              if (c >= '0' && c <= '9') return c - '0';
              if (c >= 'a' && c <= 'f') return c - 'a' + 10;
              if (c >= 'A' && c <= 'F') return c - 'A' + 10;
              return 0;
            };
            pageData[byteCount++] = (h2v(ch1) << 4) | h2v(ch2);
          }
          doodleSetPage(pageIdx, pageData, byteCount);
          doodleOpen();
          bleSend("[DRAW] P_OK:" + String(pageIdx));
          return;
        }
      }
    }
    // F. Pesan Notifikasi Aplikasi (WhatsApp, SMS, dsb)
    else {
      appName = tag;
      int colonIdx = content.indexOf(':');
      if (colonIdx != -1) {
        sender = content.substring(0, colonIdx);
        body   = content.substring(colonIdx + 1);
      } else {
        sender = "PESAN";
        body   = content;
      }
    }
  }
  // 2. FORMAT PIPA UNIVERSAL: App|Sender|Message atau Sender|Message
  else if (s.indexOf('|') != -1) {
    int firstSep = s.indexOf('|');
    int secondSep = s.indexOf('|', firstSep + 1);
    if (secondSep != -1) {
      appName = s.substring(0, firstSep);
      sender  = s.substring(firstSep + 1, secondSep);
      body    = s.substring(secondSep + 1);
    } else {
      appName = "WHATSAPP";
      sender  = s.substring(0, firstSep);
      body    = s.substring(firstSep + 1);
    }
  }
  // 3. FORMAT TEKS POLOS
  else {
    appName = (s_currentTargetOS == BLE_OS_ANDROID) ? "ANDROID" : "IPHONE";
    sender  = "PESAN";
    body    = s;
  }

  appName.trim();
  sender.trim();
  body.trim();

  // Simpan ke notifikasi aktif
  snprintf(s_currentNotif.app, sizeof(s_currentNotif.app), "%s", appName.c_str());
  snprintf(s_currentNotif.sender, sizeof(s_currentNotif.sender), "%s", sender.c_str());
  snprintf(s_currentNotif.body, sizeof(s_currentNotif.body), "%s", body.c_str());
  s_currentNotif.receivedMs = millis();
  s_currentNotif.valid = true;

  // Simpan juga ke Riwayat Pesan Masuk (Inbox)
  for (int i = BLE_INBOX_CAPACITY - 1; i > 0; --i) {
    s_inbox[i] = s_inbox[i - 1];
  }
  s_inbox[0] = s_currentNotif;
  if (s_inboxCount < BLE_INBOX_CAPACITY) s_inboxCount++;

  // Bersihkan status sentuhan lama agar notifikasi TIDAK tertutup seketika
  g_touchPending = false;
  g_navPending = false;

  // Picu pengalihan layar seketika
  s_notifStartMs = millis();
  s_notifActive  = true;
  Serial.printf("[BLE NOTIF] [%s] %s: %s (Inbox: %d)\n", s_currentNotif.app, s_currentNotif.sender, s_currentNotif.body, s_inboxCount);
}

bool bleNotifIsActive() {
  return s_notifActive;
}

void bleNotifDismiss() {
  s_notifActive = false;
}

// -----------------------------------------------------------------------------
// Gambar Ikon Vektor Prosedural Notifikasi
// -----------------------------------------------------------------------------
static void drawVectorNotificationIcon(int x, int y, const char* app, uint8_t color) {
  // 0. Ikon Wi-Fi (Setup / Notifikasi Jaringan)
  if (strstr(app, "WI") || strstr(app, "WIFI")) {
    fbDrawWifiIcon(x, y, 3, color, millis());
    return;
  }

  // 1. Ikon Chat Bubble (WhatsApp / Telegram / SMS)
  if (strstr(app, "WA") || strstr(app, "WHAT") || strstr(app, "CHAT") || strstr(app, "TELE") || strstr(app, "SMS")) {
    fbFillRoundRect(x, y + 1, 11, 8, 2, color);
    fbFillRect(x + 2, y + 8, 3, 2, color);
    // Lubang titik 3 dalam balon obrolan
    uint8_t hole = (color == C_BLACK) ? C_WHITE : C_BLACK;
    putPx(x + 3, y + 4, hole);
    putPx(x + 5, y + 4, hole);
    putPx(x + 7, y + 4, hole);
    return;
  }

  // 2. Ikon Telepon Panggilan (Call / Phone)
  if (strstr(app, "CALL") || strstr(app, "TELP") || strstr(app, "PHONE")) {
    fbFillRect(x + 2, y, 7, 3, color);
    fbFillRect(x + 4, y + 3, 3, 4, color);
    fbFillRect(x + 2, y + 7, 7, 3, color);
    return;
  }

  // 3. Ikon Amplop Surat / Email
  if (strstr(app, "MAIL") || strstr(app, "GMAIL") || strstr(app, "PESAN")) {
    fbRect(x, y + 1, 12, 8, color);
    fbLine(x + 1, y + 2, x + 6, y + 5, 1, color);
    fbLine(x + 10, y + 2, x + 5, y + 5, 1, color);
    return;
  }

  // 4. Default: Ikon Lonceng Notifikasi (Bell)
  fbFillCircle(x + 5, y + 3, 2, color);
  fbFillRect(x + 2, y + 4, 7, 4, color);
  fbHLine(x, y + 8, 11, color);
  fbFillCircle(x + 5, y + 10, 1, color);
}

// -----------------------------------------------------------------------------
// Helper Pemecah Teks Kata (Word Wrapping) Aman Sesuai Lebar OLED 128px
// -----------------------------------------------------------------------------
static void wordWrapText(const char* input, char lines[3][22]) {
  for (int i = 0; i < 3; ++i) lines[i][0] = '\0';
  if (!input) return;

  int lineIdx = 0;
  int colIdx = 0;
  int inLen = strlen(input);
  int i = 0;

  while (i < inLen && lineIdx < 3) {
    int wordLen = 0;
    while (i + wordLen < inLen && input[i + wordLen] != ' ') {
      wordLen++;
    }

    if (colIdx + wordLen > 19 && colIdx > 0) {
      lines[lineIdx][colIdx] = '\0';
      lineIdx++;
      colIdx = 0;
      if (lineIdx >= 3) break;
    }

    for (int k = 0; k < wordLen && colIdx < 19; ++k) {
      lines[lineIdx][colIdx++] = input[i + k];
    }
    i += wordLen;

    if (i < inLen && input[i] == ' ') {
      if (colIdx < 19) {
        lines[lineIdx][colIdx++] = ' ';
      }
      i++;
    }
  }
  if (lineIdx < 3) {
    lines[lineIdx][colIdx] = '\0';
  }
}

// -----------------------------------------------------------------------------
// RENDER & UPDATE LAYAR NOTIFIKASI MASUK (DESAIN KHUSUS REAL-TIME)
// -----------------------------------------------------------------------------
void bleNotifUpdate() {
  uint32_t now = millis();
  pollTouch();

  // Proteksi Waktu Tampil Minimal (Grace Period 1.5 Detik)
  // Mencegah notifikasi langsung tertutup oleh sentuhan lama atau noise sensor sentuh
  if (now - s_notifStartMs > 1500) {
    if (g_touchPending || g_navPending) {
      g_touchPending = false;
      g_navPending = false;
      bleNotifDismiss();
      return;
    }
  } else {
    // Buang event sentuhan yang terjadi selama masa proteksi 1.5 detik
    g_touchPending = false;
    g_navPending = false;
  }

  // Auto-dismiss jika durasi habis
  uint32_t elapsed = now - s_notifStartMs;
  if (elapsed >= NOTIF_DURATION_MS) {
    bleNotifDismiss();
    return;
  }

  // Render Layar Notifikasi Vektor
  displayClear();

  // 1. Header Bar Hitam Elegan (y = 0..12)
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);

  // Gambar Ikon Vektor di kiri header
  drawVectorNotificationIcon(6, 1, s_currentNotif.app, C_WHITE);

  // Nama Aplikasi (uppercase rapi)
  fbDrawString(22, 3, s_currentNotif.app, C_WHITE);

  // Tag "BARU" di kanan atas
  fbDrawString(94, 3, "[BARU]", C_WHITE);

  // 2. Baris Nama Pengirim (y = 16..24)
  fbDrawRightArrow(6, 18, C_BLACK);
  fbDrawString(16, 16, s_currentNotif.sender, C_BLACK);
  fbHLine(6, 26, 116, C_BLACK);

  // 3. Isi Teks Pesan (Word wrap 3 baris: y=29, y=39, y=49)
  char lines[3][22];
  wordWrapText(s_currentNotif.body, lines);

  for (int row = 0; row < 3; ++row) {
    if (lines[row][0] != '\0') {
      fbDrawString(6, 29 + row * 9, lines[row], C_BLACK);
    }
  }

  // 4. Progress Bar Countdown Waktu Tampil di Bawah (y = 57..61)
  fbRect(6, 57, 116, 5, C_BLACK);
  int fillW = (int)((NOTIF_DURATION_MS - elapsed) * 112 / NOTIF_DURATION_MS);
  if (fillW > 112) fillW = 112;
  if (fillW > 0) fbFillRect(8, 59, fillW, 2, C_BLACK);

  displayFlush();
  delay(15);
}

// -----------------------------------------------------------------------------
// RENDER & LOGIKA SUBMENU BLE (Pengaturan, Menunggu Koneksi, & Status)
// -----------------------------------------------------------------------------
void bleMenuUpdate() {
  uint32_t now = millis();
  pollTouch();

  // ---------------------------------------------------------------------------
  // STATUS 1: LAYAR MENUNGGU KONEKSI SMARTPHONE
  // ---------------------------------------------------------------------------
  if (s_bleSubState == BLE_SUB_CONNECTING) {
    // Jika HP terhubung, alihkan ke layar konfirmasi sukses
    if (s_deviceConnected) {
      s_bleSubState = BLE_SUB_CONNECTED_CONFIRM;
      s_connectSuccessMs = now;
      return;
    }

    // Abaikan sisa sentuhan selama 1 detik pertama (mencegah batal tidak sengaja)
    if (now - s_connectStartMs < 1000) {
      g_touchPending = false;
      g_navPending = false;
    }

    // Sentuh Sensor 1 atau Sensor 2 untuk membatalkan
    if (g_touchPending || g_navPending) {
      g_touchPending = false;
      g_navPending = false;
      s_bleSubState = BLE_SUB_MENU;
      s_lastMenuActMs = now;
      Serial.println(F("[BLE] Batal menunggu koneksi smartphone."));
      return;
    }

    // Timeout 60 detik jika belum terhubung
    if (now - s_connectStartMs > 60000) {
      s_bleSubState = BLE_SUB_MENU;
      s_lastMenuActMs = now;
      return;
    }

    displayClear();

    // 1. Header Bar
    fbFillRect(0, 0, OLED_W, 13, C_BLACK);
    const char* title = (s_currentTargetOS == BLE_OS_ANDROID) ? "SAMBUNG ANDROID" : "SAMBUNG IOS";
    int tW = fbGetStringWidth(title);
    fbDrawString((OLED_W - tW) / 2, 3, title, C_WHITE);

    // 2. Animasi Radar / Sinyal Vektor di Kiri (cx=18, cy=33)
    int phase = (now / 250) % 4;
    fbFillCircle(18, 33, 2, C_BLACK);
    if (phase >= 1) {
      fbLine(22, 29, 24, 31, 1, C_BLACK);
      fbLine(24, 31, 24, 35, 1, C_BLACK);
      fbLine(24, 35, 22, 37, 1, C_BLACK);
    }
    if (phase >= 2) {
      fbLine(26, 26, 29, 29, 1, C_BLACK);
      fbLine(29, 29, 29, 37, 1, C_BLACK);
      fbLine(29, 37, 26, 40, 1, C_BLACK);
    }
    if (phase >= 3) {
      fbLine(30, 23, 34, 27, 1, C_BLACK);
      fbLine(34, 27, 34, 39, 1, C_BLACK);
      fbLine(34, 39, 30, 43, 1, C_BLACK);
    }

    // 3. Teks Informasi di Kanan
    fbDrawString(38, 17, "Nama BLE: BMO", C_BLACK);
    fbDrawString(38, 28, "Buka Bluetooth HP", C_BLACK);

    int dots = (now / 350) % 4;
    char waitBuf[16];
    snprintf(waitBuf, sizeof(waitBuf), "Menunggu%.*s", dots, "...");
    fbDrawString(38, 39, waitBuf, C_BLACK);

    displayFlush();
    delay(20);
    return;
  }

  // ---------------------------------------------------------------------------
  // STATUS 2: KONFIRMASI BERHASIL TERHUBUNG (OTOMATIS KEMBALI KE HOME WAJAH BMO)
  // ---------------------------------------------------------------------------
  if (s_bleSubState == BLE_SUB_CONNECTED_CONFIRM) {
    displayClear();

    // Header Bar
    fbFillRect(0, 0, OLED_W, 13, C_BLACK);
    int tW = fbGetStringWidth("BLUETOOTH TERHUBUNG");
    fbDrawString((OLED_W - tW) / 2, 3, "BLUETOOTH TERHUBUNG", C_WHITE);

    // Kartu Box Centang
    fbRect(6, 16, 116, 34, C_BLACK);
    fbDrawCheckmark(14, 29, C_BLACK);
    fbDrawString(30, 21, "TERHUBUNG!", C_BLACK);
    fbDrawString(30, 33, "HP Siap Kirim Notif", C_BLACK);

    displayFlush();

    // Tampil 1.8 detik lalu kembali ke menu Bluetooth (tidak keluar ke Home)
    if (now - s_connectSuccessMs >= 1800) {
      s_bleSubState   = BLE_SUB_MENU;
      s_lastMenuActMs = now;
      g_touchPending  = false;
      g_navPending    = false;
    }
    delay(20);
    return;
  }

  // ---------------------------------------------------------------------------
  // STATUS 3: MENU SUB-HALAMAN BLE (4 Pilihan Rapi Tanpa Riwayat Pesan)
  // ---------------------------------------------------------------------------
  // Navigasi Pilihan (Sensor 2: NAV)
  if (g_navPending) {
    NavEvent ev = g_pendingNav;
    g_navPending = false;

    if (ev == NAV_TAP) {
      s_bleMenuIdx = (s_bleMenuIdx + 1) % 4;
      s_lastMenuActMs = now;
    } else if (ev == NAV_HOLD || ev == NAV_DOUBLE_TAP) {
      menuClose();
      return;
    }
  }

  // Eksekusi Pilihan (Sensor 1: ACTION / CONFIRM)
  if (g_touchPending) {
    g_touchPending = false;
    s_lastMenuActMs = now;

    if (s_bleMenuIdx == 0) {
      // 1. SAMBUNG ANDROID
      bleStart(BLE_OS_ANDROID);
      if (s_deviceConnected) {
        s_bleSubState = BLE_SUB_CONNECTED_CONFIRM;
        s_connectSuccessMs = now;
      } else {
        s_bleSubState = BLE_SUB_CONNECTING;
        s_connectStartMs = now;
      }
      return;
    } else if (s_bleMenuIdx == 1) {
      // 2. SAMBUNG IOS
      bleStart(BLE_OS_IOS);
      if (s_deviceConnected) {
        s_bleSubState = BLE_SUB_CONNECTED_CONFIRM;
        s_connectSuccessMs = now;
      } else {
        s_bleSubState = BLE_SUB_CONNECTING;
        s_connectStartMs = now;
      }
      return;
    } else if (s_bleMenuIdx == 2) {
      // 3. NYALAKAN / MATIKAN BLUETOOTH (manual oleh pengguna)
      if (s_bleActive) {
        bleStop();
      } else {
        bleStart(BLE_OS_ANDROID);
      }
      return;
    } else {
      // 4. KEMBALI KE HOME
      menuClose();
      return;
    }
  }

  // Timeout Inaktivitas 20 detik
  if (now - s_lastMenuActMs > 20000) {
    menuClose();
    return;
  }

  // Render Submenu BLE
  displayClear();

  // Header Title Bar
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);
  const char* titleTxt = s_deviceConnected ? "BLE: TERHUBUNG"
                       : (s_bleActive ? "BLE: AKTIF" : "BLE: MATI");
  int titleW = fbGetStringWidth(titleTxt);
  fbDrawString((OLED_W - titleW) / 2, 3, titleTxt, C_WHITE);
  wifiDrawHeaderIcon(116, 2, C_WHITE);

  const char* bleItems[] = {
    "SAMBUNG ANDROID",
    "SAMBUNG IOS",
    s_bleActive ? "MATIKAN BLUETOOTH" : "NYALAKAN BLUETOOTH",
    "KEMBALI KE HOME"
  };

  for (int i = 0; i < 4; ++i) {
    int y = 16 + i * 11;
    if (i == s_bleMenuIdx) {
      fbFillRoundRect(4, y, 120, 11, 2, C_BLACK);
      fbDrawRightArrow(8, y + 3, C_WHITE);
      fbDrawString(18, y + 2, bleItems[i], C_WHITE);
    } else {
      fbDrawString(18, y + 2, bleItems[i], C_BLACK);
    }
  }

  // Garis penutup bawah
  fbHLine(6, 61, 116, C_BLACK);

  displayFlush();
  delay(20);
}

// -----------------------------------------------------------------------------
// Riwayat Notifikasi Masuk (Inbox 5 Pesan Terakhir)
// -----------------------------------------------------------------------------
uint8_t bleGetInboxCount() {
  return s_inboxCount;
}

const BleNotification* bleGetInboxItem(uint8_t index) {
  if (index >= s_inboxCount) return nullptr;
  return &s_inbox[index];
}

void bleClearInbox() {
  s_inboxCount = 0;
  for (int i = 0; i < BLE_INBOX_CAPACITY; ++i) {
    s_inbox[i].valid = false;
  }
  Serial.println(F("[BLE] Riwayat notifikasi inbox dikosongkan."));
}
