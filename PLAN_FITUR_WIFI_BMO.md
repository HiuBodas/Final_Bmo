# Roadmap & Rencana Implementasi Fitur Wi-Fi BMO ESP32-C3

Dokumen ini memuat arsitektur teknis dan panduan bertahap untuk mengimplementasikan fitur berbasis koneksi Wi-Fi pada perangkat **BMO ESP32-C3** setelah menerima kredensial jaringan (SSID & Password) dari aplikasi Companion.

---

## 1. Fondasi Sistem Wi-Fi & Manajemen Memori ESP32-C3

Sebelum fitur internet dijalankan, sistem memerlukan manajemen jaringan yang stabil agar proses jaringan **tidak mengganggu animasi layar OLED (35-40 FPS)** maupun respon sensor sentuh kapasitif TTP223.

### 1.1. Penyimpanan Kredensial Permanen (NVS Flash) `[SELESAI]`
* Kredensial Wi-Fi disimpan menggunakan pustaka `Preferences` di namespace `"bmo_wifi"`:
  * `ssid` (string max 32 karakter)
  * `pass` (string max 64 karakter)
  * `auto_connect` (boolean: otomatis sambung saat booting)
* Saat firmware menerima perintah BLE `[WIFI] SSID:PASS|URL`, firmware langsung menyimpannya ke NVS dan memulai proses koneksi di background task.
* Merespon perintah BLE `[WIFI] STATUS` dan `[WIFI] DISCONNECT`.

### 1.2. Arsitektur Jaringan Non-Blocking (FreeRTOS Background Task) `[SELESAI]`
* ESP32-C3 memiliki prosesor RISC-V 160MHz.
* Seluruh operasi jaringan (`WiFi.begin()`, auto-reconnect berkala 30s) dijalankan di dalam **FreeRTOS Task terpisah** (`wifiBackgroundTask`) dengan prioritas rendah (`tskIDLE_PRIORITY + 1`) dan stack 4096 byte.
* Main loop (`loop()`) tetap fokus pada render OLED dan pembacaan sensor sentuh, bebas dari `delay()` yang dapat menyebabkan antarmuka tersendat (*stutter* tetap stabil 35-40 FPS).

### 1.3. Indikator Status Koneksi pada Layar `[SELESAI]`
* Ikon vektor Wi-Fi 3-lengkung prosedural di pojok kanan atas header OLED (128x64) pada koordinat `(116, 2)`.
* Animasi kedip (*blinking*) saat menghubungkan, solid saat terhubung, dan silang (*cross*) jika terputus/gagal. Terintegrasi di seluruh header menu (Menu Utama, Ekspresi, Riwayat Notif, Pengaturan BMO, Waktu & Timer, Pomodoro, BMO Pet, dan BLE).

---

## 2. Rencana Implementasi Kategori 1: Fitur Produktivitas & Meja Kerja

### 2.1. Jam Internet Otomatis (NTP Clock & Kalender) `[SELESAI]`
* **Tujuan**: Jam, menit, detik, dan tanggal selalu presisi tanpa perlu sinkronisasi manual dari aplikasi HP.
* **Implementasi**:
  * Menggunakan `configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com");` (WIB UTC+7).
  * Terintegrasi pada modul `NtpClock.h` & `NtpClock.cpp` serta ditampilkan pada submenu `JAM & CUACA`.

### 2.2. Live Weather & Temperature HUD `[SELESAI]`
* **Tujuan**: Menampilkan suhu kota dan kondisi cuaca dengan ikon animasi vektor prosedural.
* **Implementasi**:
  * Mengambil data ringan via HTTP dari `wttr.in/{Kota}?format=%t|%h|%C` secara background non-blocking.
  * Ikon vektor prosedural 4 kondisi di `Display.cpp`: Matahari ☀️, Awan ☁️, Hujan 🌧️, Petir ⚡.
  * Reaksi mood BMO otomatis sesuai suhu (<24°C menggigil/dingin, 25-30°C senang, >33°C kepanasan).

### 2.3. Web Dashboard BMO Lokal (`http://bmo.local`) `[SELESAI]`
* **Tujuan**: Mengontrol BMO langsung dari browser laptop atau PC kantor.
* **Implementasi**:
  * Menggunakan `WebServer` internal ESP32 + `ESPmDNS` (`http://bmo.local` & IP address).
  * UI Dark Mode Cyberpunk Mint responsif di `WebDashboard.cpp`:
    - **Mood Picker**: Tombol cepat 26 ekspresi wajah instan.
    - **Text Broadcaster**: Mengetikkan memo/pesan teks dari PC langsung muncul di layar BMO.
    - **Pomodoro Remote Control**: Memulai, menjeda, mereset sesi fokus meja kerja.
    - **Status BMO Live**: Memantau Wi-Fi, IP, Jam NTP, Cuaca, dan statistik Tamagotchi (Kenyang/Senang).
    - **Tamagotchi Actions**: Tombol Beri Makan & Elus Kepala langsung dari PC.

### 2.4. Spotify "Now Playing" Widget `[SELESAI]`
* **Tujuan**: Menampilkan judul lagu yang sedang didengarkan saat bekerja.
* **Implementasi**:
  * Modul `SpotifyWidget.h` & `SpotifyWidget.cpp`.
  * Endpoint HTTP: `POST /api/nowplaying?title=...&artist=...`.
  * Desain Layar OLED:
    - Judul lagu berjalan halus (*marquee text*).
    - Nama artist terpusat.
    - Equalizer audio 5-bar animasi prosedural melompat dinamis di bagian bawah layar.

---

## 3. Rencana Implementasi Kategori 2: Fitur Cerdas & Interaktif (AI & Bot)

### 3.1. BMO Mini AI Assistant (Gemini / ChatGPT Integration)
* **Tujuan**: Menghidupkan kepribadian BMO agar bisa menjawab pertanyaan singkat pemiliknya.
* **Alur Eksekusi**:
  1. Pengguna mengirim prompt/pertanyaan melalui aplikasi Companion atau Web Dashboard lokal.
  2. ESP32-C3 melakukan HTTPS POST ke Google Gemini API (`generativelanguage.googleapis.com`).
  3. System Prompt khusus: *"Kamu adalah BMO dari Adventure Time. Jawab dengan ramah, lucu, ceria, dan maksimal 20 kata."*
  4. Saat menunggu respon API (1-2 detik): OLED menampilkan animasi wajah BMO berpikir (`MOOD_THINKING` atau mata melirik kiri-kanan).
  5. Saat respon diterima: Teks jawaban ditampilkan dengan pemenggal kata rapi (*word-wrapping* 3 baris) diiringi ekspresi gembira (`MOOD_HAPPY` atau `MOOD_EXCITED`).

### 3.2. Bot Telegram Mandiri (Cloud Notification Inbox)
* **Tujuan**: Menerima pesan dan notifikasi penting langsung dari internet tanpa membutuhkan koneksi Bluetooth ke ponsel.
* **Mekanisme**:
  * Menggunakan polling ringan Telegram Bot API (`api.telegram.org`).
  * Pemilik membuat bot pribadi di `@BotFather` dan memasukkan Bot Token ke BMO.
  * Setiap pesan yang dikirim ke bot Telegram pribadi akan langsung menginterupsi layar BMO lengkap dengan nama pengirim dan isi pesan selama 7 detik.

---

## 4. Rencana Implementasi Kategori 3: Fitur Mini & Fun (Ticker & Counter)

### 4.1. Live Crypto, Saham, & Kurs Ticker
* **Tujuan**: Widget pemantau aset investasi di meja kerja.
* **Sumber Data**: CoinGecko API atau Binance Public API (bebas API key, format JSON ringan).
* **Fitur**:
  * Pilihan aset: Bitcoin (BTC/USD), Ethereum (ETH/USD), atau Kurs Rupiah (USD/IDR).
  * Tampilan: Simbol koin, harga terkini, dan tanda panah naik/turun (▲ / ▼).
  * Muncul bergantian di mode layar santai (*idle cycle*).

### 4.2. YouTube Subscriber & GitHub Star Counter
* **Tujuan**: Display pencapaian karya dan komunitas bagi kreator atau programmer.
* **Mekanisme**:
  * Polling ke YouTube Data API v3 (`channels?part=statistics`) atau GitHub API (`repos/{owner}/{repo}`).
  * **Celebration Event**: Saat angka bertambah, BMO memicu efek partikel selebrasi penuh (konfeti dan bintang dari `Particles.h`) dan ekspresi perayaan khusus!

### 4.3. Tamagotchi Cloud Sync & Multi-BMO Interaction
* **Tujuan**: Status virtual pet tidak hilang dan bisa berinteraksi.
* **Mekanisme**:
  * Sync status Kenyang, Senang, dan Energi ke backend MQTT / REST sederhana.
  * Fitur "Sapa Teman": BMO bisa menerima sinyal sapaan dari BMO lain melalui protokol MQTT publik.

---

## 5. Tahapan Roadmap Pengembangan (Phased Milestones)

| Fase | Target Fitur | Estimasi Kompleksitas | Status |
| :--- | :--- | :--- | :--- |
| **Fase 1** | **Wi-Fi Manager Core & NTP Clock**: Penyimpanan NVS, auto-reconnect background task, dan jam internet otomatis | Rendah - Sedang | Selesai (Seksi 1.1 - 2.1) |
| **Fase 2** | **Live Weather HUD, Web Dashboard Laptop, & Spotify**: Ikon cuaca vektor, reaksi suhu wajah BMO, Web Dashboard `http://bmo.local`, dan Spotify Now Playing | Sedang | Selesai (Seksi 2.2 - 2.4) |
| **Fase 3** | **Mini AI Companion & Ticker**: Integrasi Gemini AI response singkat, Ticker Crypto/Saham, dan Telegram Bot | Lanjutan | Tahap Berikutnya |

---

*File rencana ini dapat dijadikan acuan utama saat mulai mengeksekusi integrasi kode ke firmware ESP32-C3.*
