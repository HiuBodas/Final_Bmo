// =============================================================================
// WebDashboard.cpp - Implementasi Web Dashboard BMO Lokal http://bmo.local
// Bagian dari Roadmap BMO ESP32 (Seksi 2.3)
// Dashboard Web Responsif HTML5/CSS3 Disimpan dalam Flash PROGMEM
// =============================================================================
#include "WebDashboard.h"
#include "WifiManager.h"
#include "NtpClock.h"
#include "WeatherManager.h"
#include "SpotifyWidget.h"
#include "Tamagotchi.h"
#include "TimeMenu.h"
#include "BleManager.h"
#include "Moods.h"

#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>

static WebServer s_server(80);
static bool      s_serverRunning = false;

// -----------------------------------------------------------------------------
// HTML Dashboard Mini Responsif Disimpan di Flash PROGMEM
// -----------------------------------------------------------------------------
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>BMO Web Dashboard</title>
  <style>
    :root {
      --bg: #0C131F;
      --surface: #152238;
      --surface-elev: #1C2C47;
      --mint: #5DCBC2;
      --blue: #2A6AEB;
      --text: #F8FAFC;
      --text-muted: #94A3B8;
      --border: rgba(255,255,255,0.12);
      --online: #10B981;
      --radius: 14px;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background: var(--bg);
      color: var(--text);
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      padding: 16px;
      max-width: 900px;
      margin: 0 auto;
    }
    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 16px;
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: var(--radius);
      margin-bottom: 16px;
    }
    .logo-group { display: flex; align-items: center; gap: 12px; }
    .logo {
      width: 40px; height: 40px; border-radius: 10px;
      background: var(--mint); color: #000;
      display: flex; align-items: center; justify-content: center;
      font-weight: 900; font-size: 18px;
    }
    h1 { font-size: 20px; font-weight: 700; color: var(--text); }
    .badge {
      font-size: 11px; padding: 4px 10px; border-radius: 20px;
      background: rgba(16,185,129,0.15); color: var(--online);
      border: 1px solid rgba(16,185,129,0.3); font-weight: 600;
    }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(260px, 1fr)); gap: 14px; margin-bottom: 16px; }
    .card {
      background: var(--surface); border: 1px solid var(--border);
      border-radius: var(--radius); padding: 16px;
    }
    .card h2 { font-size: 14px; font-weight: 600; color: var(--text-muted); margin-bottom: 12px; text-transform: uppercase; letter-spacing: 0.5px; }
    .stat-val { font-size: 24px; font-weight: 800; color: var(--mint); margin-bottom: 4px; }
    .stat-desc { font-size: 12px; color: var(--text-muted); }
    .mood-grid {
      display: grid; grid-template-columns: repeat(auto-fill, minmax(80px, 1fr)); gap: 8px;
    }
    button {
      background: var(--surface-elev); color: var(--text);
      border: 1px solid var(--border); border-radius: 8px;
      padding: 10px 8px; font-size: 12px; font-weight: 600;
      cursor: pointer; transition: all 0.2s;
    }
    button:hover { background: var(--mint); color: #000; border-color: var(--mint); transform: translateY(-1px); }
    button.btn-primary { background: var(--blue); border-color: var(--blue); color: #fff; }
    button.btn-primary:hover { background: #3B82F6; color: #fff; }
    .form-group { margin-bottom: 10px; }
    label { display: block; font-size: 12px; color: var(--text-muted); margin-bottom: 4px; }
    input {
      width: 100%; padding: 10px 12px; border-radius: 8px;
      background: rgba(0,0,0,0.3); border: 1px solid var(--border);
      color: var(--text); font-size: 13px; outline: none;
    }
    input:focus { border-color: var(--mint); }
    .toast {
      position: fixed; bottom: 20px; right: 20px;
      background: var(--mint); color: #000; font-weight: 700;
      padding: 12px 18px; border-radius: 10px; display: none;
      box-shadow: 0 4px 15px rgba(0,0,0,0.5); z-index: 99;
    }
    .pet-bars { display: flex; flex-direction: column; gap: 8px; margin-bottom: 12px; }
    .bar-row { display: flex; align-items: center; justify-content: space-between; font-size: 12px; }
    .bar-wrap { width: 120px; height: 8px; background: rgba(0,0,0,0.3); border-radius: 4px; overflow: hidden; }
    .bar-fill { height: 100%; background: var(--mint); border-radius: 4px; transition: width 0.3s; }
  </style>
</head>
<body>
  <header>
    <div class="logo-group">
      <div class="logo">BMO</div>
      <div>
        <h1>BMO Control Center</h1>
        <div class="stat-desc" id="hdrSub">http://bmo.local</div>
      </div>
    </div>
    <div class="badge" id="wifiBadge">ONLINE</div>
  </header>

  <!-- Baris Info Cepat -->
  <div class="grid">
    <div class="card">
      <h2>Waktu Global & Tanggal (NTP)</h2>
      <div class="stat-val" id="timeVal">--:--:--</div>
      <div class="stat-desc" id="dateVal">Memuat tanggal...</div>
    </div>
    <div class="card">
      <h2>Cuaca & Suhu HUD</h2>
      <div class="stat-val" id="tempVal">--&deg;C</div>
      <div class="stat-desc" id="weatherVal">Memuat cuaca...</div>
    </div>
    <div class="card">
      <h2>Status Tamagotchi Pet</h2>
      <div class="pet-bars">
        <div class="bar-row">
          <span>Kenyang</span>
          <div class="bar-wrap"><div class="bar-fill" id="fillHunger" style="width: 0%"></div></div>
        </div>
        <div class="bar-row">
          <span>Senang</span>
          <div class="bar-wrap"><div class="bar-fill" id="fillHappy" style="width: 0%"></div></div>
        </div>
      </div>
      <div style="display:flex; gap:6px;">
        <button style="flex:1" onclick="petAction('feed')">&#127828; Beri Makan</button>
        <button style="flex:1" onclick="petAction('pet')">&#129303; Elus Kepala</button>
      </div>
    </div>
  </div>

  <!-- Mood Picker & Text Broadcaster -->
  <div class="grid">
    <div class="card" style="grid-column: span 2;">
      <h2>Mood Picker (Pilih Ekspresi BMO Instan)</h2>
      <div class="mood-grid" id="moodGrid"></div>
    </div>
    <div class="card">
      <h2>Kirim Memo Teks ke Layar</h2>
      <div class="form-group">
        <label>Judul / Tag</label>
        <input type="text" id="msgApp" value="MEMO PC">
      </div>
      <div class="form-group">
        <label>Isi Memo</label>
        <input type="text" id="msgBody" placeholder="Ketik catatan di sini...">
      </div>
      <button class="btn-primary" style="width:100%;" onclick="sendMessage()">Kirim ke Layar BMO</button>
    </div>
  </div>

  <!-- Pomodoro & Spotify Controller -->
  <div class="grid">
    <div class="card">
      <h2>Remote Pomodoro Meja</h2>
      <div class="stat-val" id="pomoTime">--:--</div>
      <div class="stat-desc" id="pomoStatus" style="margin-bottom:12px;">Mode Fokus</div>
      <div style="display:flex; gap:8px;">
        <button style="flex:1" class="btn-primary" onclick="pomoAction('start')">Mulai</button>
        <button style="flex:1" onclick="pomoAction('pause')">Jeda</button>
        <button style="flex:1" onclick="pomoAction('reset')">Reset</button>
      </div>
    </div>
    <div class="card">
      <h2>Spotify "Now Playing" Sender</h2>
      <div class="form-group">
        <label>Judul Lagu</label>
        <input type="text" id="spotTitle" placeholder="Contoh: Everything Stays">
      </div>
      <div class="form-group">
        <label>Nama Artis</label>
        <input type="text" id="spotArtist" placeholder="Contoh: Marceline">
      </div>
      <button class="btn-primary" style="width:100%" onclick="sendSpotify()">Tampilkan di Layar BMO</button>
    </div>
  </div>

  <div class="toast" id="toast">Berhasil dikirim!</div>

  <script>
    const MOODS = [
      "COOL", "HAPPY", "LAUGH", "LOVE", "WINK", "IDLE", "BLINK", "TALK",
      "LISTEN", "FOCUSED", "THINKING", "SURPRISE", "EXCITED", "HUNGRY",
      "COLD", "HOT", "SICK", "DIZZY", "GLITCH", "CONFUSED", "BORED",
      "SAD", "SCARED", "ANGRY", "WAKE", "SLEEPY"
    ];

    const grid = document.getElementById("moodGrid");
    MOODS.forEach(m => {
      const b = document.createElement("button");
      b.textContent = m;
      b.onclick = () => setMood(m);
      grid.appendChild(b);
    });

    function showToast(msg) {
      const t = document.getElementById("toast");
      t.textContent = msg;
      t.style.display = "block";
      setTimeout(() => { t.style.display = "none"; }, 2000);
    }

    async function setMood(m) {
      await fetch('/api/mood?m=' + m, { method: 'POST' });
      showToast('Wajah BMO diubah ke: ' + m);
    }

    async function sendMessage() {
      const app = document.getElementById('msgApp').value;
      const body = document.getElementById('msgBody').value;
      if (!body) return;
      await fetch(`/api/message?app=${encodeURIComponent(app)}&body=${encodeURIComponent(body)}`, { method: 'POST' });
      showToast('Memo teks ditampilkan di BMO!');
      document.getElementById('msgBody').value = '';
    }

    async function petAction(act) {
      await fetch('/api/tamagotchi?action=' + act, { method: 'POST' });
      showToast(act === 'feed' ? 'BMO diberi makan!' : 'BMO dielus!');
      fetchStatus();
    }

    async function pomoAction(act) {
      await fetch('/api/pomodoro?action=' + act, { method: 'POST' });
      showToast('Perintah Pomodoro terkirim!');
      fetchStatus();
    }

    async function sendSpotify() {
      const t = document.getElementById('spotTitle').value;
      const a = document.getElementById('spotArtist').value;
      if (!t) return;
      await fetch(`/api/nowplaying?title=${encodeURIComponent(t)}&artist=${encodeURIComponent(a)}`, { method: 'POST' });
      showToast('Widget Spotify diaktifkan di BMO!');
    }

    async function fetchStatus() {
      try {
        const res = await fetch('/api/status');
        const data = await res.json();
        document.getElementById('timeVal').textContent = data.time || '--:--:--';
        document.getElementById('dateVal').textContent = `${data.day || ''}, ${data.date || ''}`;
        document.getElementById('tempVal').innerHTML = `${data.temp}&deg;C`;
        document.getElementById('weatherVal').textContent = `${data.weather} | ${data.humidity}% RH (${data.city || 'Lokal'})`;
        document.getElementById('hdrSub').textContent = `IP: ${data.ip} | RSSI: ${data.rssi} dBm`;
        
        document.getElementById('fillHunger').style.width = (data.hunger || 0) + '%';
        document.getElementById('fillHappy').style.width = (data.happiness || 0) + '%';

        const pM = Math.floor((data.pomoRemain || 0) / 60);
        const pS = (data.pomoRemain || 0) % 60;
        document.getElementById('pomoTime').textContent = `${String(pM).padStart(2,'0')}:${String(pS).padStart(2,'0')}`;
        document.getElementById('pomoStatus').textContent = data.pomoWork ? 'Sesi Fokus (#1/4)' : 'Sesi Istirahat';
      } catch (_) {}
    }

    setInterval(fetchStatus, 3000);
    fetchStatus();
  </script>
</body>
</html>
)rawliteral";

// -----------------------------------------------------------------------------
// Route Handlers
// -----------------------------------------------------------------------------
static void handleRoot() {
  s_server.send_P(200, "text/html", INDEX_HTML);
}

static void handleApiStatus() {
  int h, m, s;
  ntpGetTime(h, m, s);

  char json[384];
  snprintf(
    json, sizeof(json),
    "{\"wifi\":\"%s\",\"ip\":\"%s\",\"rssi\":%d,\"time\":\"%s\",\"date\":\"%s\",\"day\":\"%s\","
    "\"temp\":%d,\"humidity\":%d,\"weather\":\"%s\",\"city\":\"%s\","
    "\"hunger\":%d,\"happiness\":%d,\"energy\":%d,"
    "\"pomoRemain\":%lu,\"pomoWork\":%s,\"spotifyTitle\":\"%s\",\"spotifyArtist\":\"%s\"}",
    wifiIsConnected() ? "CONNECTED" : "DISCONNECTED",
    wifiGetIP().c_str(),
    wifiGetRSSI(),
    ntpGetTimeStr().c_str(),
    ntpGetDateStr().c_str(),
    ntpGetDayStr().c_str(),
    weatherGetTemp(),
    weatherGetHumidity(),
    weatherGetCondition().c_str(),
    weatherGetCity().c_str(),
    tamagotchiGetHunger(),
    tamagotchiGetHappiness(),
    tamagotchiGetEnergy(),
    timeMenuGetPomodoroRemain(),
    timeMenuIsPomodoroWork() ? "true" : "false",
    spotifyGetTitle().c_str(),
    spotifyGetArtist().c_str()
  );

  s_server.send(200, "application/json", json);
}

static void handleApiMood() {
  if (s_server.hasArg("m")) {
    String mName = s_server.arg("m");
    mName.toUpperCase();
    mName.trim();

    Mood target = MOOD_HAPPY;
    if (mName == "COOL") target = MOOD_COOL;
    else if (mName == "HAPPY") target = MOOD_HAPPY;
    else if (mName == "LAUGH") target = MOOD_LAUGH;
    else if (mName == "LOVE") target = MOOD_LOVE;
    else if (mName == "WINK") target = MOOD_WINK;
    else if (mName == "IDLE") target = MOOD_IDLE;
    else if (mName == "BLINK") target = MOOD_BLINK;
    else if (mName == "TALK") target = MOOD_TALK;
    else if (mName == "LISTEN") target = MOOD_LISTEN;
    else if (mName == "FOCUSED") target = MOOD_FOCUSED;
    else if (mName == "THINKING") target = MOOD_THINKING;
    else if (mName == "SURPRISE") target = MOOD_SURPRISE;
    else if (mName == "EXCITED") target = MOOD_EXCITED;
    else if (mName == "HUNGRY") target = MOOD_HUNGRY;
    else if (mName == "COLD") target = MOOD_COLD;
    else if (mName == "HOT") target = MOOD_HOT;
    else if (mName == "SICK") target = MOOD_SICK;
    else if (mName == "DIZZY") target = MOOD_DIZZY;
    else if (mName == "GLITCH") target = MOOD_GLITCH;
    else if (mName == "CONFUSED") target = MOOD_CONFUSED;
    else if (mName == "BORED") target = MOOD_BORED;
    else if (mName == "SAD") target = MOOD_SAD;
    else if (mName == "SCARED") target = MOOD_SCARED;
    else if (mName == "ANGRY") target = MOOD_ANGRY;
    else if (mName == "WAKE") target = MOOD_WAKE;
    else if (mName == "SLEEPY") target = MOOD_SLEEPY;

    bleTriggerMood(target);
    s_server.send(200, "text/plain", "OK");
    return;
  }
  s_server.send(400, "text/plain", "Missing mood param 'm'");
}

static void handleApiMessage() {
  String app = s_server.hasArg("app") ? s_server.arg("app") : "MEMO PC";
  String body = s_server.hasArg("body") ? s_server.arg("body") : "";
  String sender = s_server.hasArg("sender") ? s_server.arg("sender") : "DASHBOARD";

  if (body.length() > 0) {
    blePushNotification(app.c_str(), sender.c_str(), body.c_str());
    s_server.send(200, "text/plain", "OK");
  } else {
    s_server.send(400, "text/plain", "Empty message body");
  }
}

static void handleApiPomodoro() {
  if (s_server.hasArg("action")) {
    String act = s_server.arg("action");
    if (act == "start") {
      timeMenuStartPomodoro();
    } else if (act == "pause") {
      timeMenuPausePomodoro();
    } else if (act == "reset") {
      timeMenuResetPomodoro();
    }
    s_server.send(200, "text/plain", "OK");
    return;
  }
  s_server.send(400, "text/plain", "Missing action");
}

static void handleApiNowPlaying() {
  String title  = s_server.hasArg("title") ? s_server.arg("title") : "";
  String artist = s_server.hasArg("artist") ? s_server.arg("artist") : "";

  if (title.length() > 0) {
    spotifySetTrack(title, artist);
    spotifyOpen(); // Otomatis buka layar Spotify di BMO
    s_server.send(200, "text/plain", "OK");
  } else {
    s_server.send(400, "text/plain", "Empty title");
  }
}

static void handleApiTamagotchi() {
  if (s_server.hasArg("action")) {
    String act = s_server.arg("action");
    if (act == "feed") {
      tamagotchiFeed();
    } else if (act == "pet") {
      tamagotchiPet();
    }
    s_server.send(200, "text/plain", "OK");
    return;
  }
  s_server.send(400, "text/plain", "Missing action");
}

// -----------------------------------------------------------------------------
// Inisialisasi & Pengaturan Server
// -----------------------------------------------------------------------------
void webDashboardInit() {
  s_serverRunning = false;
}

void webDashboardStart() {
  if (s_serverRunning) return;

  // Mulai mDNS agar bisa diakses via http://bmo.local
  if (MDNS.begin("bmo")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println(F("[Web] mDNS aktif: http://bmo.local"));
  }

  // Registrasi Routes
  s_server.on("/", HTTP_GET, handleRoot);
  s_server.on("/api/status", HTTP_GET, handleApiStatus);
  s_server.on("/api/mood", HTTP_POST, handleApiMood);
  s_server.on("/api/message", HTTP_POST, handleApiMessage);
  s_server.on("/api/pomodoro", HTTP_POST, handleApiPomodoro);
  s_server.on("/api/nowplaying", HTTP_POST, handleApiNowPlaying);
  s_server.on("/api/tamagotchi", HTTP_POST, handleApiTamagotchi);

  s_server.begin();
  s_serverRunning = true;
  Serial.printf("[Web] Web Dashboard berjalan di: http://%s dan http://bmo.local\n",
                WiFi.localIP().toString().c_str());
}

void webDashboardStop() {
  if (s_serverRunning) {
    s_server.stop();
    MDNS.end();
    s_serverRunning = false;
    Serial.println(F("[Web] Web Dashboard dinonaktifkan."));
  }
}

void webDashboardHandle() {
  if (s_serverRunning) {
    s_server.handleClient();
  }
}
