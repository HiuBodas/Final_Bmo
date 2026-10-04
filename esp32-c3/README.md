# BMO ESP32-C3: Standalone Expressions & Time Suite

Firmware mandiri (standalone) BMO untuk mikrokontroler ESP32-C3 Super Mini dengan Layar OLED 0.96 inci (I2C SSD1306 128x64) dan konfigurasi Dual Sensor Sentuh Kapasitif TTP223.

Seluruh elemen visual dibangun 100% menggunakan grafik vektor prosedural matematis tanpa file gambar maupun array bitmap flash PROGMEM. Sistem beroperasi mandiri tanpa dependensi cloud, audio eksternal, atau pustaka pihak ketiga di luar Arduino core ESP32.

---

## Spesifikasi Teknis Hardware

- Mikrokontroler: ESP32-C3 Super Mini (RISC-V 32-bit Single-Core @ 160 MHz)
- Layar: OLED Monokrom 0.96 inci I2C SSD1306 (Resolusi 128x64 piksel)
- Sensor Input: 2x Modul Sensor Sentuh Kapasitif TTP223
- Catu Daya: 3.3V DC (diambil langsung dari pin 3V3 ESP32-C3)
- Protokol Komunikasi: I2C Hardware (Kecepatan Fast-Mode 400 kHz)

---

## Skema Pengkabelan (Wiring Pinout)

Berikut adalah diagram koneksi pin antara board ESP32-C3 Super Mini, modul Layar OLED SSD1306, dan kedua Sensor Sentuh Kapasitif TTP223:

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

Catatan Konfigurasi Pin:
- Pin I2C default ESP32-C3 berada pada GPIO 8 (SDA) dan GPIO 9 (SCL).
- Pin GPIO 20 dan GPIO 21 secara bawaan dihubungkan ke peripheral internal UART0. Firmware ini secara otomatis melepaskan fungsi peripheral tersebut menggunakan `gpio_reset_pin()` pada fungsi `touchInit()` sehingga kedua pin beroperasi murni sebagai input GPIO digital sensitif.
- Modul TTP223 tipe Active-HIGH maupun Active-LOW langsung dideteksi polaritas idle-nya secara otomatis saat booting.

---

## Konsep Desain Antarmuka Layar (OLED 128x64)

Layar beroperasi dalam tema monokrom kontras tinggi:
- Latar Belakang (Background): Putih Solid (Buffer OLED = 1).
- Garis & Teks (Foreground): Hitam Pekat (Buffer OLED = 0).
- Desain Layar Bersih: Tidak ada petunjuk teks tombol yang memadati layar (seperti keterangan pin atau footer tombol). Seluruh area 128x64 dimanfaatkan secara proporsional untuk tipografi jam digital besar, progress bar, header, dan ekspresi vektor.

---

## Panduan Lengkap Navigasi Dual-Sensor

Sistem menggunakan 2 sensor sentuh fisik dengan pembagian tugas yang konsisten:

1. **Sensor 1 (GPIO 20) - Sensor Aksi & Interaksi Wajah**:
   - Di Halaman Home: Merespons sentuhan fisik wajah BMO (Colek, Elus, Peluk Hati, Gelitik).
   - Di Menu: Bertindak sebagai tombol Konfirmasi (OK / Pilih).
   - Di Stopwatch: Memulai atau Menjeda penghitungan waktu (Start / Pause).
   - Di Timer & Pomodoro: Memulai / Menjeda hitungan mundur, atau mematikan alarm saat waktu habis.

2. **Sensor 2 (GPIO 21) - Sensor Navigasi Menu**:
   - Di Halaman Home: Ketuk 1x untuk membuka Menu Utama. Tahan > 800ms untuk membuka kunci ekspresi.
   - Di Menu & Submenu: Ketuk 1x untuk menggeser kursor ke item berikutnya.
   - Tahan (> 800ms) atau Ketuk 2x Cepat: Membatalkan atau kembali ke tingkat menu sebelumnya.

---

## Diagram Alur Menu Bertingkat (State Machine)

```text
 [HOME: Wajah BMO Hidup / Terkunci]
      |
      | (Sensor 2: Ketuk 1x)
      v
 [MENU UTAMA (8 Pilihan)]
   |-- 1. PILIH EKSPRESI  --> [SUBMENU EKSPRESI (26 Mood Teks)]
   |-- 2. WAKTU & TIMER   --> [SUBMENU WAKTU (Stopwatch, Timer, Pomodoro)]
   |-- 3. BMO TAMAGOTCHI  --> [MODE VIRTUAL PET (Kenyang, Senang, Energi)]
   |-- 4. BMO ARCADE GAME --> [MINI GAME RUNNER (Lompat Rintangan + High Score)]
   |-- 5. RIWAYAT NOTIF   --> [INBOX NOTIFIKASI (Baca 5 Pesan Terakhir dari HP)]
   |-- 6. BLUETOOTH & NOTIF-> [KONEKSI BLE (Android / iOS / Status)]
   |-- 7. PENGATURAN BMO  --> [PENGATURAN (Kontras, Flip 180, Personality Engine)]
   `-- 8. KEMBALI KE HOME --> [KEMBALI KE TAMPILAN WAJAH UTAMA]
```

---

## Tabel Rinci Kontrol Sensor pada Tiap Halaman

### 1. Halaman Home (Wajah Ekspresi Hidup)

| Sensor | Gestur Sentuhan | Aksi yang Dijalankan |
| :--- | :--- | :--- |
| Sensor 1 | Ketukan Singkat (< 250 ms) | Reaksi Colek (Quick Poke): Kaget melompat, melirik kiri-kanan mencari pelaku. |
| Sensor 1 | Sentuhan Sedang (250 ms - 1.5 s) | Reaksi Elus (Hold/Pet): Mata terpejam rileks, mendengkur, pipi merona. |
| Sensor 1 | Sentuhan Lama (> 1.5 s) | Reaksi Peluk Hati (Long Hold/Love): Ledakan partikel hati dan mata hati berdenyut. |
| Sensor 1 | Tiga Ketukan Cepat berturut-turut | Reaksi Gelitik (Tickle): Tertawa terpingkal-pingkal dengan konfeti bintang. |
| Sensor 2 | Ketuk 1x | Membuka Menu Utama BMO (8 Pilihan). |
| Sensor 2 | Tahan (> 800 ms) | Membuka kunci ekspresi (Unlock) jika sebelumnya dikunci dari Menu Pilih Ekspresi. |

---

### 2. Menu Utama (8 Pilihan dengan Scrolling Viewport)

Tampilan layar menyajikan 4 baris opsi sekaligus dengan jendela geser (*scrolling viewport*) dan scrollbar vertikal di sisi kanan layar:
1. `PILIH EKSPRESI`
2. `WAKTU & TIMER`
3. `BMO TAMAGOTCHI`
4. `BMO ARCADE GAME`
5. `RIWAYAT NOTIF`
6. `BLUETOOTH & NOTIF`
7. `PENGATURAN BMO`
8. `KEMBALI KE HOME`

| Sensor | Gestur Sentuhan | Aksi yang Dijalankan |
| :--- | :--- | :--- |
| Sensor 2 | Ketuk 1x | Menggeser kursor ke bawah secara melingkar (1 s/d 8). Layar otomatis bergulir rapi. |
| Sensor 1 | Ketuk 1x | Memilih / Masuk ke menu yang sedang ditunjuk kursor. |
| Sensor 2 | Tahan (> 800 ms) | Langsung menutup menu dan kembali ke Halaman Home. |
| Sensor 2 | Dobel Ketuk | Langsung menutup menu dan kembali ke Halaman Home. |
| Otomatis | Timeout 20 detik | Otomatis menutup menu jika tidak ada interaksi selama 20 detik. |

---

### 3. Submenu 1: PILIH EKSPRESI

Tampilan layar menyajikan 4 baris teks ekspresi yang bersih dan elegan (tanpa teks bertumpuk/angka indeks berlebih), bar highlight, dan scrollbar vertikal di sisi kanan layar.

| Sensor | Gestur Sentuhan | Aksi yang Dijalankan |
| :--- | :--- | :--- |
| Sensor 2 | Ketuk 1x | Menggeser pilihan ke ekspresi berikutnya secara berurutan. |
| Sensor 1 | Ketuk 1x | Konfirmasi pilihan (OK). Menampilkan kotak dialog konfirmasi, lalu mengunci ekspresi tersebut dan kembali ke Halaman Home seketika. |
| Sensor 2 | Tahan (> 800 ms) | Membatalkan pemilihan dan kembali ke Menu Utama. |
| Sensor 2 | Dobel Ketuk | Membatalkan pemilihan dan kembali ke Menu Utama. |

---

### 4. Submenu 2: Suite Waktu (Menu Pilihan Alat)

Tampilan layar menampilkan daftar alat produktivitas:
- STOPWATCH
- HITUNG MUNDUR
- POMODORO
- KEMBALI

| Sensor | Gestur Sentuhan | Aksi yang Dijalankan |
| :--- | :--- | :--- |
| Sensor 2 | Ketuk 1x | Menggeser kursor ke pilihan berikutnya. |
| Sensor 1 | Ketuk 1x | Masuk ke alat waktu yang dipilih. |
| Sensor 2 | Tahan (> 800 ms) | Kembali ke Menu Utama. |

---

### 5. Stopwatch

Tampilan layar: Header bersih `STOPWATCH`, angka digital besar format MM:SS.d (Menit : Detik . Persepuluh detik), serta indikator status operasional di bagian bawah (`SIAP`, detak animasi saat aktif, atau `TERJEDA - S2 RESET`).

| Sensor | Gestur Sentuhan | Aksi yang Dijalankan |
| :--- | :--- | :--- |
| Sensor 1 | Ketuk 1x | Mulai menghitung (Start) atau Menjeda hitungan (Pause). |
| Sensor 2 | Ketuk 1x (saat jeda) | Reset waktu kembali ke 00:00.0. |
| Sensor 2 | Tahan (> 800 ms) | Keluar dari Stopwatch dan kembali ke Submenu Waktu. |
| Sensor 2 | Dobel Ketuk | Keluar dari Stopwatch dan kembali ke Submenu Waktu. |

---

### 6. Hitung Mundur (Countdown Timer)

Tampilan layar: Header bersih dan terpusat `HITUNG MUNDUR` (bebas dari teks bertumpuk), angka digital besar format MM:SS, progress bar proporsional, dan status operasional (`TAP S2: ATUR MENIT`, `BERJALAN`, `TERJEDA - S2 RESET`, atau `WAKTU HABIS!`).

| Sensor | Gestur Sentuhan | Aksi yang Dijalankan |
| :--- | :--- | :--- |
| Sensor 1 | Ketuk 1x | Mulai hitung mundur atau Menjeda hitungan. |
| Sensor 1 | Ketuk 1x (saat alarm) | Mematikan dan me-reset alarm waktu habis kembali ke durasi preset. |
| Sensor 2 | Ketuk 1x (saat standby) | Berganti preset durasi (1 -> 3 -> 5 -> 10 -> 15 -> 25 menit). |
| Sensor 2 | Ketuk 1x (saat jeda) | Reset hitung mundur kembali ke durasi preset awal. |
| Sensor 2 | Tahan (> 800 ms) | Keluar dari Timer dan kembali ke Submenu Waktu. |
| Sensor 2 | Dobel Ketuk | Keluar dari Timer dan kembali ke Submenu Waktu. |

---

### 7. Pomodoro (Durasi Menit Fleksibel & Dapat Diatur)

Tampilan layar: Header siklus (#1 s/d #4), lencana status sesi [FOKUS / ISTIRAHAT], angka digital besar format MM:SS, progress bar horizontal, dan teks status petunjuk (`TAP S2: ATUR MENIT`, `SESI FOKUS`, `SESI ISTIRAHAT`, `TERJEDA - S2 RESET`, atau `ISTIRAHAT DULU!`).

| Sensor | Gestur Sentuhan | Aksi yang Dijalankan |
| :--- | :--- | :--- |
| Sensor 2 | Ketuk 1x (saat standby) | **Mengatur durasi menit fokus terlebih dahulu** (pilihan preset: 5, 10, 15, 20, 25, 30, 45, 50, 60 menit). |
| Sensor 1 | Ketuk 1x | Mulai sesi fokus atau menjeda sesi aktif. |
| Sensor 2 | Ketuk 1x (saat jeda) | Reset sesi yang terjeda kembali ke durasi awal. |
| Sensor 1 | Ketuk 1x (saat alarm habis) | Mematikan alarm dan otomatis melanjutkan ke sesi berikutnya (Fokus -> Istirahat, atau Istirahat -> Fokus berikutnya). |
| Sensor 2 | Tahan (> 800 ms) | Menghentikan Pomodoro dan keluar kembali ke Submenu Waktu. |
| Sensor 2 | Dobel Ketuk | Menghentikan Pomodoro dan keluar kembali ke Submenu Waktu. |

---

### 7. Submenu 3: Mode BMO Tamagotchi (Virtual Pet Mandiri)

Mode Virtual Pet terisolasi sebagai menu tersendiri, **tidak menggantikan** interaksi sentuh alami maupun siklus ekspresi normal di halaman Home saat keluar:
- **Parameter Vital Pet**:
  - **Kenyang (Hunger)** : 0 - 100% (berkurang otomatis secara berkala).
  - **Senang (Happiness)** : 0 - 100% (berkurang jika tidak dirawat/dielus).
  - **Energi (Energy)** : 0 - 100% (berkurang seiring aktivitas).
- **Tampilan Wajah Interaktif**: Ekspresi wajah BMO berubah dinamis mengikuti parameter (Lapar -> `MOOD_HUNGRY`, Sedih -> `MOOD_SAD`, Sangat Senang -> `MOOD_EXCITED`/`COOL`).
- **HUD Mini**: Bagian atas layar menampilkan bar hitam elegan dengan ikon hati `<3`, status kenyang, dan tag mode `[PET]`.

| Sensor | Gestur Sentuhan | Aksi yang Dijalankan |
| :--- | :--- | :--- |
| Sensor 1 | Ketuk Singkat (< 250 ms) | **Beri Makan BMO**: Menambah Kenyang +20%, Senang +5%, memicu animasi makan nyam-nyam & popup feedback. |
| Sensor 1 | Tahan (> 500 ms) | **Elus / Sayang BMO**: Menambah Senang +25%, Energi +5%, memicu animasi hati penuh cinta (`MOOD_LOVE`). |
| Sensor 2 | Ketuk 1x | **Beralih Tampilan**: Berpindah antara Wajah Pet Live dan Status Detail (3 progress bar vital & hitungan rawat). |
| Sensor 2 | Tahan (> 800 ms) / Dobel Ketuk | **Keluar dari Tamagotchi**: Kembali ke Menu Utama BMO. Siklus normal & interaksi Home tetap utuh. |

---

### 8. Submenu 4: BMO Arcade Game (Procedural Jump Runner)

Mini game arcade monokrom vektor cepat (35-40 FPS) terinspirasi game lari rintangan:
- **Karakter Mini-BMO**: Kotak BMO berwajah dengan animasi kaki berlari yang berayun dan melipat saat melompat di udara.
- **Rintangan Prosedural**: Spike segitiga tunggal, balok glitch/kaktus tinggi, dan spike ganda yang melaju dari kanan.
- **Penyimpanan Skor Permanen**: Skor tertinggi (*High Score*) otomatis disimpan ke memori Flash ESP32 NVS via library `Preferences` sehingga tidak hilang saat perangkat dimatikan.
- **Tampilan Skor**: Layar header menampilkan skor berjalan `S:` dan skor rekor `HI:`.

| Sensor | Gestur Sentuhan | Aksi yang Dijalankan |
| :--- | :--- | :--- |
| Sensor 1 | Ketuk 1x (di Layar Mulai / Game Over) | Memulai permainan baru / mengulang kembali (*Play Again*). |
| Sensor 1 | Ketuk 1x (saat bermain) | **Melompat (Jump)** menghindari rintangan di tanah. |
| Sensor 2 | Tahan (> 800 ms) / Dobel Ketuk | Keluar dari mini game kembali ke Menu Utama. |

---

### 9. Submenu 5: Riwayat Notifikasi Masuk (Inbox 5 Pesan Terakhir)

Menyimpan 5 pesan notifikasi smartphone terakhir dalam memori *circular ring buffer*:
- **Daftar Pesan**: Menampilkan nama aplikasi (WhatsApp, Instagram, dll), pengirim, dan ringkasan.
- **Tampilan Detail**: Menampilkan isi pesan lengkap dengan pemenggal kata (*word-wrapping*) hingga 4 baris rapi.
- **Opsi Bersihkan**: Menyediakan opsi `KOSONGKAN INBOX` untuk menghapus seluruh riwayat pesan dengan feedback centang.

| Sensor | Gestur Sentuhan | Aksi yang Dijalankan |
| :--- | :--- | :--- |
| Sensor 2 | Ketuk 1x (di Daftar) | Menggeser kursor pilihan pesan. |
| Sensor 1 | Ketuk 1x (pada pesan) | Membuka Tampilan Detail membaca isi pesan lengkap. |
| Sensor 1 / 2 | Ketuk 1x (di Tampilan Detail) | Kembali ke daftar pesan. |
| Sensor 2 | Tahan (> 800 ms) | Keluar kembali ke Menu Utama. |

---

### 10. Submenu 7: Pengaturan BMO (Settings)

Menu konfigurasi perangkat keras yang tersimpan permanen di Flash NVS:
1. `KONTRAS` : Pilihan tingkat kecerahan OLED (`[RENDAH]`, `[SEDANG]`, `[MAX]`).
2. `PUTAR LAYAR` : Orientasi rotasi layar OLED 180° (`[0°]` normal atau `[180°]` terbalik jika casing dipasang terbalik).
3. `IDLE` : Pilihan mode wajah idle (`[RUTIN]` 26 siklus mood atau `[MOCHI]` personality engine dinamis).
4. `KEMBALI KE MENU` : Menyimpan dan kembali ke menu utama.

| Sensor | Gestur Sentuhan | Aksi yang Dijalankan |
| :--- | :--- | :--- |
| Sensor 2 | Ketuk 1x | Menggeser kursor pengaturan (1 s/d 4). |
| Sensor 1 | Ketuk 1x | Mengubah nilai pengaturan (langsung diterapkan ke layar OLED secara instan). |
| Sensor 2 | Tahan (> 800 ms) | Keluar kembali ke Menu Utama. |

---

### 11. Submenu 6: Koneksi Bluetooth (BLE) & Notifikasi Real-time Smartphone

Firmware mengintegrasikan profil standar **Nordic UART Service (NUS)** yang kompatibel secara instan dengan smartphone Android dan iPhone (iOS) tanpa memerlukan bonding/pairing rumit.

- **Nama Perangkat BLE**: `BMO`
- **Pilihan Menu BLE**:
  1. `SAMBUNG ANDROID` : Mengaktifkan advertising BLE dan menampilkan layar menunggu koneksi Android.
  2. `SAMBUNG IOS` : Mengaktifkan advertising BLE dan menampilkan layar menunggu koneksi iPhone (iOS).
  3. `MATIKAN BLUETOOTH` : Mematikan radio BLE untuk menghemat daya.
  4. `KEMBALI KE HOME` : Kembali ke tampilan wajah ekspresi BMO.

- **Alur Koneksi Otomatis**:
  - Saat memilih `SAMBUNG ANDROID` atau `SAMBUNG IOS`, layar BMO beralih ke **Tampilan Menunggu Koneksi** dengan animasi radar sinyal vektor dan instruksi nama perangkat.
  - Begitu smartphone terhubung, layar seketika menampilkan konfirmasi `[✓] TERHUBUNG!` selama 1.8 detik, kemudian otomatis kembali ke **Home (Wajah Ekspresi BMO)**.
  - Pengguna dapat membatalkan proses menunggu kapan saja dengan menyentuh Sensor 1 atau Sensor 2.

- **Interupsi Layar Ekspresi Real-time**:
  - Saat BMO sedang berekspresi atau bernafas santai di Home, kedatangan paket pesan notifikasi BLE seketika **menginterupsi animasi wajah** dan langsung menampilkan **desain notifikasi vektor elegan**:
    - **Header Bar**: Ikon vektor spesifik (balon obrolan titik 3 untuk WhatsApp/SMS, telepon untuk panggilan, amplop untuk email, atau lonceng notifikasi) + Nama Aplikasi + Tag `[BARU]`.
    - **Pengirim**: Nama kontak/pengirim dengan tanda panah dan garis pembatas tegas.
    - **Isi Pesan**: Teks pesan dengan pemenggal kata otomatis (*word wrapping*) rapi hingga 3 baris.
    - **Progress Bar Bawah**: Hitung mundur otomatis selama 7 detik.
  - **Tutup Notifikasi**: Ketuk **Sensor 1** atau **Sensor 2** kapan saja (atau tunggu 7 detik) untuk menutup notifikasi dan BMO akan kembali secara instan ke ekspresi wajahnya.

- **Format Pengiriman Data via BLE (Aplikasi HP / Serial BLE)**:
  - Format 3 Bagian: `Aplikasi|Pengirim|Pesan` *(Contoh: `WhatsApp|Budi|Halo BMO, meeting jam 3 ya!`)*
  - Format 2 Bagian: `Pengirim|Pesan` *(Otomatis terdeteksi sebagai WhatsApp)*
  - Format 1 Bagian: `Pesan Teks Polos` *(Otomatis terdeteksi sebagai Notifikasi Sistem)*

---

## Daftar 26 Ekspresi Vektor BMO

Semua ekspresi digambar secara dinamis melalui persamaan matematis vektor:

| No | Nama Ekspresi | Deskripsi Visual |
| :---: | :--- | :--- |
| 01 | COOL | Kacamata hitam visor Pit Viper dengan animasi pantulan kilau diagonal bergerak. |
| 02 | HAPPY | Mata bulan sabit ramah, pipi bintik merona, senyum gembira. |
| 03 | LAUGH | Mulut terbuka tawa lebar, tubuh berguncang riang. |
| 04 | LOVE | Kedua mata berbentuk hati besar berdenyut, partikel hati melayang. |
| 05 | WINK | Mengedipkan mata kanan disertai seringai lucu. |
| 06 | IDLE | Wajah santai dengan irama nafas sinusoidal, lirikan mata, dan kedipan lembut. |
| 07 | BLINK | Animasi kedipan mata halus alami. |
| 08 | TALK | Mulut bergerak membuka-menutup ritmis menyerupai percakapan. |
| 09 | LISTEN | Gelombang visual audio aktif di kedua sisi telinga/layar. |
| 10 | FOCUSED | Mata menyipit konsentrasi penuh, alis tegas, butiran keringat usaha keras. |
| 11 | THINKING | Mata spinner berputar memproses pikiran, mulut datar tenang. |
| 12 | SURPRISE | Mata bulat melotot kaget, pupil mengecil bergetar, mulut membulat huruf O. |
| 13 | EXCITED | Bintang berkilau di sudut layar, tubuh melompat antusias. |
| 14 | HUNGRY | Mata melirik ke bawah, lidah menjulur keluar, tetesan air liur lapar. |
| 15 | COLD | Tubuh menggigil kencang, gigi gemeretak bergaris, butiran salju turun. |
| 16 | HOT | Wajah lemas kepanasan, uap mengepul dari kepala, meneteskan keringat. |
| 17 | SICK | Mulut bergelombang zigzag tidak enak badan, alis sayu lemas. |
| 18 | DIZZY | Mata spiral berputar pusing, wajah oleng kiri-kanan berkunang-kunang. |
| 19 | GLITCH | Garis-garis layar datamosh rusak, mata bersilang X. |
| 20 | CONFUSED | Mata titik kecil heran, mulut berombak, tanda tanya melayang. |
| 21 | BORED | Mata melirik samping malas, mulut datar, huruf z Z Z mengambang. |
| 22 | SAD | Alis melengkung sedih, butiran air mata mengalir turun di pipi. |
| 23 | SCARED | Mata melotot ketakutan, pupil gemetar, keringat dingin membasahi wajah. |
| 24 | ANGRY | Alis tajam menukik ke bawah, mata tajam, wajah bergetar menahan amarah. |
| 25 | WAKE | Menguap lebar dari mata terpejam rapat hingga terbuka lebar segar bugar. |
| 26 | SLEEPY | Kelopak mata sayu tertutup sebagian naik-turun mengikuti irama nafas mengantuk. |

---

## Panduan Kompilasi & Pengunggahan Firmware (Arduino IDE)

1. Buka Arduino IDE.
2. Buka berkas `esp32-c3/esp32-c3.ino`. Semua berkas pendukung (`Config.h`, `Display.*`, `Touch.*`, `Moods.*`, `Menu.*`, `TimeMenu.*`, `Particles.*`) akan terbuka otomatis di tab editor.
3. Konfigurasi Target Board pada menu **Tools**:
   - Board: **ESP32C3 Dev Module**
   - Port: Pilih port COM perangkat ESP32-C3 Anda
   - USB CDC On Boot: **Enabled** (Wajib aktif agar serial monitor dan program berjalan normal pada ESP32-C3 Super Mini)
   - Flash Size: **4MB**
   - Partition Scheme: **Default 4MB with spiffs**
   - CPU Frequency: **160MHz**
4. Klik tombol **Verify** untuk memeriksa kompilasi, lalu klik **Upload** untuk mengisi firmware ke chip ESP32-C3.
