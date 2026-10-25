# BMO Companion (Flutter Android & iOS)

Aplikasi pendamping cross-platform (Android & iOS) untuk robot desktop BMO. Didesain dengan estetika Pearl + Cobalt Glassmorphism (kartu transparan, blur halus, tipografi Plus Jakarta Sans, dan tata letak modern).

---

## Fitur Utama

1. **Beranda & Status BMO (Home)**:
   - Status koneksi Bluetooth Low Energy (Online / Offline).
   - Avatar ekspresi wajah BMO mini yang interaktif.
   - Sinkronisasi jam digital & baterai HP ke layar BMO dengan 1 ketukan.
   - Pengirim pesan cepat untuk ditampilkan di layar OLED BMO.
   - Riwayat log aktivitas Bluetooth real-time.

2. **BMO Simulator & Remote Wajah**:
   - Avatar Layar Virtual BMO: Mengikuti ekspresi yang dipilih secara real-time.
   - 26 Mood Wajah Asli BMO:
     - Senang (`MOOD:HAPPY`) | Keren (`MOOD:COOL`) | Tertawa (`MOOD:LAUGH`) | Cinta (`MOOD:LOVE`) | Kedip (`MOOD:WINK`)
     - Santai (`MOOD:IDLE`) | Kedip-kedip (`MOOD:BLINK`) | Bicara (`MOOD:TALK`) | Mendengar (`MOOD:LISTEN`) | Fokus (`MOOD:FOCUSED`)
     - Mikir (`MOOD:THINKING`) | Kaget (`MOOD:SURPRISE`) | Semangat (`MOOD:EXCITED`) | Lapar (`MOOD:HUNGRY`) | Dingin (`MOOD:COLD`)
     - Kepanasan (`MOOD:HOT`) | Sakit (`MOOD:SICK`) | Pusing (`MOOD:DIZZY`) | Glitch (`MOOD:GLITCH`) | Bingung (`MOOD:CONFUSED`)
     - Bosan (`MOOD:BORED`) | Sedih (`MOOD:SAD`) | Takut (`MOOD:SCARED`) | Marah (`MOOD:ANGRY`) | Bangun (`MOOD:WAKE`) | Tidur (`MOOD:SLEEPY`)
   - 4 Stimulasi Sensor Sentuh (TTP223):
     - Colek / Poke (`TOUCH:POKE`)
     - Elus / Pet (`TOUCH:HOLD`)
     - Peluk / Long Hold (`TOUCH:LONG_HOLD`)
     - Gelitik / Tickle (`TOUCH:TICKLE`)

3. **Wi-Fi Provisioning via Bluetooth**:
   - Form input SSID dan Password Wi-Fi, serta URL Dashboard BMO.
   - Mengirimkan kredensial langsung via Bluetooth ke BMO tanpa perlu gonta-ganti Wi-Fi hotspot ponsel.
   - Cek status IP dan koneksi Wi-Fi BMO.

4. **Pengaturan & Bluetooth Manager**:
   - Pemindai otomatis perangkat Bluetooth BMO.
   - Filter khusus perangkat BMO di sekitar.
   - Tombol Sambungkan & Putuskan koneksi yang stabil.

---

## Kompilasi & Build

### 1. Kompilasi Otomatis via GitHub Actions
File workflow tersedia di `.github/workflows/build_flutter.yml`:
- Saat kode di-push ke repository, GitHub Actions otomatis membangun:
  - **Android**: File APK (Release) siap diunduh di tab Actions Artifacts dan Releases.
  - **iOS**: File Runner.app siap untuk sideloading (AltStore, Sideloadly, TrollStore).

### 2. Kompilasi Manual Lokal

#### Android:
```bash
cd apk
flutter pub get
flutter build apk --release
```
File APK berada di: `build/app/outputs/flutter-apk/app-release.apk`

#### iOS:
```bash
cd apk
flutter pub get
flutter build ios --release --no-codesign
```

---

## Privasi & Keamanan
- **100% Offline & Lokal**: Komunikasi antara HP dan BMO berjalan langsung via Bluetooth Low Energy (Nordic UART Service UUID `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`).
- **Bebas Dependensi Cloud**: Tidak ada data yang dikirim ke server pihak ketiga.
