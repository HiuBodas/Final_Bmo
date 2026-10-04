# Final BMO - Robot Desktop ESP32-C3 & Flutter Companion

Sistem robot desktop BMO interaktif berbasis mikrokontroler ESP32-C3 Super Mini dengan Layar OLED 0.96 inci (I2C SSD1306 128x64) dan aplikasi pendamping smartphone Flutter (Android APK & iOS).

Seluruh grafik pada layar OLED dibuat 100% menggunakan persamaan vektor prosedural tanpa file bitmap, gambar eksternal, maupun dependensi cloud pihak ketiga.

---

## Struktur Repositori

Repositori ini terdiri dari dua komponen utama:

```text
Final_Bmo/
|-- .github/
|   `-- workflows/
|       `-- build_flutter.yml   # CI/CD otomatis untuk build APK Android & iOS Runner
|-- apk/                        # Aplikasi Flutter Companion (Android & iOS)
|   |-- android/                # Konfigurasi native Android
|   |-- ios/                    # Konfigurasi native iOS
|   |-- lib/                    # Sumber kode Flutter (UI, BLE Service, State)
|   `-- pubspec.yaml            # Dependensi paket Flutter
|-- esp32-c3/                   # Firmware mikrokontroler ESP32-C3
|   |-- esp32-c3.ino            # Program utama, inisialisasi hardware & idle loop
|   |-- Config.h                # Konfigurasi pinout hardware & parameter OLED
|   |-- Display.h / .cpp        # Driver SSD1306, framebuffer, dan primitif vektor
|   |-- Moods.h / .cpp          # Generator 26 ekspresi wajah & personality engine
|   |-- Touch.h / .cpp          # Driver debouncing dual-sensor kapasitif TTP223
|   |-- Menu.h / .cpp           # Sistem menu bertingkat 8 pilihan dengan viewport scroll
|   |-- TimeMenu.h / .cpp       # Suite alat waktu (Stopwatch, Timer, Pomodoro)
|   |-- Tamagotchi.h / .cpp     # Mode Virtual Pet mandiri (Kenyang, Senang, Energi)
|   |-- Game.h / .cpp           # Mini game arcade jump runner dengan High Score NVS
|   |-- BleManager.h / .cpp     # Modul BLE Nordic UART Service & notifikasi inbox
|   `-- Particles.h / .cpp      # Efek visual partikel hati, bintang, dan konfeti
|-- .gitignore                  # Filter berkas build Flutter, Android, dan IDE
`-- README.md                   # Dokumentasi utama proyek
```

---

## Spesifikasi Hardware & Skema Pinout

Perangkat menggunakan mikrokontroler ESP32-C3 Super Mini dengan catu daya 3.3V:

```text
       Layar OLED 0.96" I2C                     ESP32-C3 Super Mini
     +----------------------+                 +---------------------+
     | VCC                  |---------------->| 3.3V                |
     | GND                  |---------------->| GND                 |
     | SCL                  |---------------->| GPIO 9              |
     | SDA                  |---------------->| GPIO 8              |
     +----------------------+                 |                     |
      Sensor 1: Aksi & Wajah                  |                     |
     +----------------------+                 |                     |
     | VCC                  |---------------->| 3.3V                |
     | GND                  |---------------->| GND                 |
     | I/O (SIG)            |---------------->| GPIO 20             |
     +----------------------+                 |                     |
      Sensor 2: Navigasi Menu                 |                     |
     +----------------------+                 |                     |
     | VCC                  |---------------->| 3.3V                |
     | GND                  |---------------->| GND                 |
     | I/O (SIG)            |---------------->| GPIO 21             |
     +----------------------+                 +---------------------+
```

Catatan Teknis Pinout:
- I2C SSD1306 berjalan pada Fast-Mode 400 kHz pada pin GPIO 8 (SDA) dan GPIO 9 (SCL).
- Pin GPIO 20 dan 21 telah diisolasi dari UART0 melalui `gpio_reset_pin()` sehingga berfungsi murni sebagai digital touch input.
- Dual modul TTP223 mendukung deteksi otomatis polaritas Active-HIGH maupun Active-LOW saat booting.

---

## Sistem Navigasi Menu BMO (8 Pilihan)

Menu Utama dilengkapi dengan *scrolling viewport* 4 baris dan scrollbar vertikal di sisi kanan layar:

```text
 [HOME: Wajah BMO Hidup / Terkunci]
      |
      | (Sensor 2: Ketuk 1x)
      v
 [MENU UTAMA]
   |-- 1. PILIH EKSPRESI   --> Submenu 26 Ekspresi Wajah Vektor Teks
   |-- 2. WAKTU & TIMER    --> Suite Stopwatch, Hitung Mundur, dan Pomodoro
   |-- 3. BMO TAMAGOTCHI   --> Mode Virtual Pet Mandiri (Kenyang, Senang, Energi)
   |-- 4. BMO ARCADE GAME  --> Mini Game Jump Runner Vektor + High Score NVS
   |-- 5. RIWAYAT NOTIF    --> Inbox 5 Notifikasi Terakhir dari Smartphone
   |-- 6. BLUETOOTH & NOTIF --> Manajemen Koneksi BLE (Android, iOS, Off)
   |-- 7. PENGATURAN BMO   --> Kontras OLED, Putar Layar (180 deg), Idle Engine
   `-- 8. KEMBALI KE HOME  --> Kembali ke Tampilan Wajah BMO
```

### Panduan Kontrol Sensor

1. **Sensor 1 (GPIO 20) - Sensor Wajah & Aksi**:
   - Di Halaman Home: Merespons sentuhan fisik (Colek, Elus, Peluk Hati, Gelitik).
   - Di Menu & Submenu: Memilih item (Konfirmasi / OK).
   - Di Tamagotchi: Ketuk singkat untuk Memberi Makan (+20% kenyang); Tahan (>500ms) untuk Mengelus (+25% senang).
   - Di Arcade Game: Ketuk untuk Melompat (Jump) atau Mengulang Permainan.
   - Di Notifikasi Aktif: Menutup notifikasi lebih awal.

2. **Sensor 2 (GPIO 21) - Sensor Navigasi**:
   - Di Halaman Home: Ketuk 1x untuk membuka Menu Utama; Tahan (>800ms) untuk melepas penguncian ekspresi.
   - Di Menu & Submenu: Ketuk 1x untuk menggeser kursor ke bawah secara melingkar.
   - Tahan (>800ms) atau Dobel Ketuk: Batal / Kembali ke tingkat menu sebelumnya.
   - Di Tamagotchi: Ketuk 1x untuk beralih antara Wajah Pet Live dan Status Detail; Tahan (>800ms) untuk keluar ke Menu Utama.

---

## Fitur Utama Firmware ESP32-C3

1. **BMO Tamagotchi (Virtual Pet Mandiri)**:
   - Terisolasi sebagai menu tersendiri tanpa menggantikan interaksi wajah bawaan di Home.
   - Tiga parameter vital: Kenyang, Senang, dan Energi (0 - 100%).
   - Tampilan wajah live yang bereaksi otomatis sesuai kondisi pet, disertai mini HUD di bagian atas layar.

2. **BMO Arcade Runner (Mini Game Vektor)**:
   - Game melompat rintangan monokrom 35-40 FPS dengan animasi karakter mini-BMO.
   - Rintangan prosedural (spike segitiga tunggal, balok kaktus tinggi, dan spike ganda).
   - High Score disimpan otomatis ke memori Flash ESP32 NVS (`Preferences`) sehingga rekor tidak hilang saat daya dimatikan.

3. **Riwayat Notifikasi Inbox**:
   - Menyimpan 5 pesan notifikasi smartphone terakhir dalam ring buffer memori.
   - Dilengkapi layar baca detail dengan pemenggal kata (*word-wrapping*) hingga 4 baris rapi.
   - Tersedia opsi untuk mengosongkan riwayat notifikasi.

4. **Pengaturan Perangkat Keras (Settings)**:
   - Kontras Layar: Rendah (35), Sedang (128), Maksimal (255).
   - Rotasi Layar: Normal (0 derajat) atau Terbalik (180 derajat).
   - Mode Idle: Siklus 26 Mood Rutin atau Mochi Personality Engine dinamis.
   - Seluruh pengaturan disimpan permanen di Flash NVS.

5. **Koneksi BLE & Notifikasi Real-time**:
   - Profil standar Nordic UART Service (NUS).
   - Notifikasi masuk seketika menginterupsi layar idle dengan header aplikasi, nama pengirim, isi pesan, dan progress bar hitung mundur 7 detik.

---

## Cara Mendapatkan & Membangun File APK

### 1. Unduh Otomatis via GitHub Actions (Rekomendasi)

Setiap kali kode di-push ke repositori GitHub, GitHub Actions otomatis membangun file APK:
1. Buka repositori GitHub: `https://github.com/HiuBodas/Final_Bmo`
2. Masuk ke tab **Actions**.
3. Pilih workflow run terbaru pada **Build BMO Flutter App (Android APK & iOS)**.
4. Pada bagian **Artifacts** di bawah halaman run, unduh berkas **BMO-Companion-Android-APK**.
5. Ekstrak file zip dan instal file `app-release.apk` ke ponsel Android Anda.
6. Berkas APK juga tersedia pada tab **Releases** jika rilis tag dibuat.

### 2. Kompilasi Manual Lokal (Jika Flutter Terinstal)

Pastikan Flutter SDK (versi >= 3.22.x) dan Android SDK telah terpasang di komputer:

```bash
# Masuk ke direktori apk
cd apk

# Ambil dependensi
flutter pub get

# Bangun file APK release
flutter build apk --release
```

Berkas APK hasil build akan berlokasi di:
`apk/build/app/outputs/flutter-apk/app-release.apk`

---

## Panduan Flash Firmware ke ESP32-C3

1. Pasang Arduino IDE (versi 2.x).
2. Instal board package **esp32** via Boards Manager (oleh Espressif Systems).
3. Buka berkas `esp32-c3/esp32-c3.ino`.
4. Konfigurasi menu **Tools**:
   - Board: **ESP32C3 Dev Module**
   - Port: Pilih port COM USB mikrokontroler
   - USB CDC On Boot: **Enabled**
   - Flash Size: **4MB**
   - Partition Scheme: **Default 4MB with spiffs**
   - CPU Frequency: **160MHz**
5. Klik **Verify**, kemudian klik **Upload**.
