import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../services/ble_service.dart';
import '../theme/bmo_theme.dart';
import '../widgets/glass_container.dart';

class WifiProvisioningTab extends StatefulWidget {
  const WifiProvisioningTab({super.key});

  @override
  State<WifiProvisioningTab> createState() => _WifiProvisioningTabState();
}

class _WifiProvisioningTabState extends State<WifiProvisioningTab> {
  final BleService _ble = BleService();
  final TextEditingController _ssidController = TextEditingController();
  final TextEditingController _passController = TextEditingController();
  final TextEditingController _cityController = TextEditingController(text: 'Jakarta');
  final TextEditingController _urlController = TextEditingController();

  // Spotify Controller
  final TextEditingController _spotTitleController = TextEditingController(text: 'Theme Song');
  final TextEditingController _spotArtistController = TextEditingController(text: 'Adventure Time');

  bool _obscurePassword = true;
  String _statusFeedback = 'Siap mengirim pengaturan Wi-Fi ke BMO.';

  @override
  void initState() {
    super.initState();
    _loadSavedForm();

    // Otomatis minta status koneksi BMO jika BLE sudah terhubung
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (_ble.isConnected) {
        _ble.requestWifiStatus();
      }
    });
  }

  void _loadSavedForm() async {
    final prefs = await SharedPreferences.getInstance();
    setState(() {
      _ssidController.text = prefs.getString('bmo_saved_ssid') ?? '';
      _cityController.text = prefs.getString('bmo_saved_city') ?? 'Jakarta';
      _urlController.text = prefs.getString('bmo_saved_url') ?? '';
    });
  }

  @override
  void dispose() {
    _ssidController.dispose();
    _passController.dispose();
    _cityController.dispose();
    _urlController.dispose();
    _spotTitleController.dispose();
    _spotArtistController.dispose();
    super.dispose();
  }

  void _saveWifiToBmo() async {
    final ssid = _ssidController.text.trim();
    final pass = _passController.text.trim();
    final city = _cityController.text.trim();
    final url = _urlController.text.trim();

    if (ssid.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Nama Wi-Fi (SSID) tidak boleh kosong!')),
      );
      return;
    }

    HapticFeedback.lightImpact();
    // Format payload ke ESP32: [WIFI] SSID:PASS|URL
    final payload = '[WIFI] $ssid:$pass|$url';

    setState(() {
      _statusFeedback = 'Mengirim data Wi-Fi ke BMO via Bluetooth...';
    });

    final success = await _ble.sendCommand(
      payload,
      logTitle: 'Kirim Wi-Fi',
      logDetail: 'SSID: $ssid',
    );

    if (success) {
      final prefs = await SharedPreferences.getInstance();
      await prefs.setString('bmo_saved_ssid', ssid);
      if (city.isNotEmpty) {
        await prefs.setString('bmo_saved_city', city);
        // Kirim lokasi kota cuaca ke BMO (Seksi 2.2)
        await Future.delayed(const Duration(milliseconds: 250));
        await _ble.setWeatherCity(city);
      }
      if (url.isNotEmpty) await prefs.setString('bmo_saved_url', url);

      setState(() {
        _statusFeedback = 'Terkirim! BMO sedang menghubungkan ke Wi-Fi "$ssid".';
      });
    } else {
      setState(() {
        _statusFeedback = 'Gagal mengirim. Pastikan BMO sudah terhubung Bluetooth.';
      });
    }
  }

  void _checkWifiStatus() {
    HapticFeedback.lightImpact();
    setState(() {
      _statusFeedback = 'Meminta status Wi-Fi dari BMO...';
    });
    _ble.requestWifiStatus();
  }

  void _disconnectWifi() async {
    final confirm = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        backgroundColor: BmoTheme.surfaceElevated,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        title: const Text('Putuskan Wi-Fi?', style: TextStyle(color: BmoTheme.textPrimary)),
        content: const Text(
          'BMO akan memutuskan sambungan Wi-Fi dan mematikan radio nirkabel.',
          style: TextStyle(color: BmoTheme.textSecondary),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('Batal', style: TextStyle(color: BmoTheme.textMuted)),
          ),
          ElevatedButton(
            onPressed: () => Navigator.pop(ctx, true),
            style: ElevatedButton.styleFrom(
              backgroundColor: BmoTheme.statusOffline,
              foregroundColor: Colors.white,
            ),
            child: const Text('Putuskan'),
          ),
        ],
      ),
    );

    if (confirm == true) {
      HapticFeedback.lightImpact();
      setState(() {
        _statusFeedback = 'Memutuskan koneksi Wi-Fi BMO...';
      });
      await _ble.disconnectWifi();
    }
  }

  void _sendSpotifyToBmo() async {
    final title = _spotTitleController.text.trim();
    final artist = _spotArtistController.text.trim();

    if (title.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Judul lagu tidak boleh kosong!')),
      );
      return;
    }

    HapticFeedback.lightImpact();
    final success = await _ble.sendSpotifyTrack(title, artist);
    if (success) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Lagu "$title" ditampilkan di layar BMO!')),
      );
      setState(() {
        _statusFeedback = 'Widget Spotify aktif di BMO: "$title - $artist"';
      });
    }
  }

  Widget _buildWifiStatusCard(bool isBleConnected) {
    Color statusColor;
    IconData statusIcon;
    String statusTitle;
    String statusSubtitle;

    if (!isBleConnected) {
      statusColor = BmoTheme.textMuted;
      statusIcon = Icons.bluetooth_disabled_rounded;
      statusTitle = 'Bluetooth Terputus';
      statusSubtitle = 'Hubungkan ponsel ke BMO di tab Beranda terlebih dahulu.';
    } else {
      switch (_ble.wifiState) {
        case 'CONNECTED':
          statusColor = BmoTheme.statusOnline;
          statusIcon = Icons.wifi_rounded;
          statusTitle = 'BMO Terhubung ke Wi-Fi';
          statusSubtitle = _ble.wifiIp != null
              ? 'IP Address: ${_ble.wifiIp}'
              : 'Jaringan lokal aktif dan siap digunakan.';
          break;
        case 'CONNECTING':
          statusColor = BmoTheme.statusConnecting;
          statusIcon = Icons.wifi_find_rounded;
          statusTitle = 'Sedang Menghubungkan...';
          statusSubtitle = 'BMO sedang mencoba menyambung ke Access Point.';
          break;
        case 'FAILED':
          statusColor = BmoTheme.statusOffline;
          statusIcon = Icons.wifi_off_rounded;
          statusTitle = 'Gagal Terhubung';
          statusSubtitle = 'Periksa kembali SSID dan kata sandi jaringan.';
          break;
        case 'DISCONNECTED':
        case 'OFF':
          statusColor = BmoTheme.textSecondary;
          statusIcon = Icons.wifi_off_rounded;
          statusTitle = 'Wi-Fi BMO Nonaktif';
          statusSubtitle = 'Kirim kredensial atau klik "Cek Status".';
          break;
        default:
          statusColor = BmoTheme.bmoMint;
          statusIcon = Icons.wifi_lock_rounded;
          statusTitle = 'Status Belum Diperbarui';
          statusSubtitle = 'Klik tombol "Cek Status" di bawah.';
          break;
      }
    }

    return GlassContainer(
      fill: statusColor.withOpacity(0.12),
      borderColor: statusColor.withOpacity(0.35),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Container(
                padding: const EdgeInsets.all(10),
                decoration: BoxDecoration(
                  color: statusColor.withOpacity(0.2),
                  shape: BoxShape.circle,
                ),
                child: Icon(statusIcon, color: statusColor, size: 24),
              ),
              const SizedBox(width: 14),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      statusTitle,
                      style: TextStyle(
                        fontSize: 15,
                        fontWeight: FontWeight.bold,
                        color: statusColor,
                      ),
                    ),
                    const SizedBox(height: 3),
                    Text(
                      statusSubtitle,
                      style: TextStyle(
                        fontSize: 12,
                        color: Colors.white.withOpacity(0.85),
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),

          // Tampilkan IP Badge dan Akses Dashboard jika tersambung
          if (isBleConnected && _ble.wifiState == 'CONNECTED') ...[
            const SizedBox(height: 12),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
              decoration: BoxDecoration(
                color: Colors.black.withOpacity(0.25),
                borderRadius: BorderRadius.circular(10),
                border: BorderSide(color: statusColor.withOpacity(0.3)),
              ),
              child: Column(
                children: [
                  Row(
                    children: [
                      const Icon(Icons.lan_rounded, color: BmoTheme.statusOnline, size: 18),
                      const SizedBox(width: 8),
                      Expanded(
                        child: Text(
                          _ble.wifiIp != null ? 'http://${_ble.wifiIp}' : 'http://bmo.local',
                          style: const TextStyle(
                            fontSize: 13,
                            fontWeight: FontWeight.w600,
                            color: BmoTheme.textPrimary,
                            letterSpacing: 0.5,
                          ),
                        ),
                      ),
                      InkWell(
                        onTap: () {
                          final url = _ble.wifiIp != null ? 'http://${_ble.wifiIp}' : 'http://bmo.local';
                          Clipboard.setData(ClipboardData(text: url));
                          HapticFeedback.lightImpact();
                          ScaffoldMessenger.of(context).showSnackBar(
                            const SnackBar(content: Text('Alamat IP BMO disalin ke clipboard!')),
                          );
                        },
                        borderRadius: BorderRadius.circular(6),
                        child: const Padding(
                          padding: EdgeInsets.all(4),
                          child: Icon(Icons.copy_rounded, color: BmoTheme.bmoMint, size: 16),
                        ),
                      ),
                    ],
                  ),
                  const Divider(color: BmoTheme.glassBorder, height: 16),
                  Row(
                    children: [
                      const Icon(Icons.laptop_chromebook_rounded, color: BmoTheme.bmoMint, size: 18),
                      const SizedBox(width: 8),
                      const Expanded(
                        child: Text(
                          'Web Dashboard: http://bmo.local',
                          style: TextStyle(fontSize: 12, color: BmoTheme.textSecondary),
                        ),
                      ),
                      InkWell(
                        onTap: () {
                          Clipboard.setData(const ClipboardData(text: 'http://bmo.local'));
                          HapticFeedback.lightImpact();
                          ScaffoldMessenger.of(context).showSnackBar(
                            const SnackBar(content: Text('URL "http://bmo.local" disalin! Buka di browser laptop.')),
                          );
                        },
                        borderRadius: BorderRadius.circular(6),
                        child: const Padding(
                          padding: EdgeInsets.all(4),
                          child: Icon(Icons.copy_rounded, color: BmoTheme.primaryLight, size: 16),
                        ),
                      ),
                    ],
                  ),
                ],
              ),
            ),
          ],
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return ListenableBuilder(
      listenable: _ble,
      builder: (context, _) {
        final isConnected = _ble.isConnected;
        final isWifiConnected = _ble.wifiState == 'CONNECTED';

        return ListView(
          padding: const EdgeInsets.fromLTRB(20, 16, 20, 100),
          children: [
            // Title
            Text(
              'Pengaturan Wi-Fi BMO',
              style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                    fontWeight: FontWeight.bold,
                    color: BmoTheme.textPrimary,
                  ),
            ),
            const SizedBox(height: 4),
            const Text(
              'Kirim kredensial Wi-Fi & lokasi cuaca via Bluetooth tanpa ribet buka Captive Portal.',
              style: TextStyle(fontSize: 14, color: BmoTheme.textSecondary),
            ),
            const SizedBox(height: 16),

            // 1. Live Wi-Fi Status Card
            _buildWifiStatusCard(isConnected),
            const SizedBox(height: 14),

            // 2. Info Card BLE Provisioning
            GlassContainer(
              fill: const Color(0x1A2563EB),
              borderColor: BmoTheme.primaryLight.withOpacity(0.3),
              child: Row(
                children: [
                  const Icon(Icons.bolt, color: BmoTheme.bmoMint, size: 28),
                  const SizedBox(width: 12),
                  Expanded(
                    child: Text(
                      'Metode BLE Provisioning ini menggantikan hotspot "BMO-Setup". Anda tidak perlu memutus Wi-Fi ponsel!',
                      style: TextStyle(
                        fontSize: 13,
                        color: Colors.white.withOpacity(0.9),
                        height: 1.35,
                      ),
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 16),

            // 3. Form Glass Card
            GlassContainer(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'Kredensial Jaringan & Lokasi',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.w600,
                      color: BmoTheme.textPrimary,
                    ),
                  ),
                  const SizedBox(height: 16),

                  // SSID
                  TextField(
                    controller: _ssidController,
                    style: const TextStyle(color: BmoTheme.textPrimary),
                    decoration: InputDecoration(
                      labelText: 'Nama Wi-Fi (SSID)',
                      prefixIcon: const Icon(Icons.wifi, color: BmoTheme.bmoMint, size: 20),
                      labelStyle: const TextStyle(color: BmoTheme.textMuted),
                      filled: true,
                      fillColor: Colors.black.withOpacity(0.25),
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(14),
                        borderSide: const BorderSide(color: BmoTheme.glassBorder),
                      ),
                    ),
                  ),
                  const SizedBox(height: 12),

                  // Password
                  TextField(
                    controller: _passController,
                    obscureText: _obscurePassword,
                    style: const TextStyle(color: BmoTheme.textPrimary),
                    decoration: InputDecoration(
                      labelText: 'Kata Sandi Wi-Fi',
                      prefixIcon: const Icon(Icons.lock_outline, color: BmoTheme.bmoMint, size: 20),
                      suffixIcon: IconButton(
                        icon: Icon(
                          _obscurePassword ? Icons.visibility_off : Icons.visibility,
                          color: BmoTheme.textMuted,
                          size: 20,
                        ),
                        onPressed: () {
                          setState(() {
                            _obscurePassword = !_obscurePassword;
                          });
                        },
                      ),
                      labelStyle: const TextStyle(color: BmoTheme.textMuted),
                      filled: true,
                      fillColor: Colors.black.withOpacity(0.25),
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(14),
                        borderSide: const BorderSide(color: BmoTheme.glassBorder),
                      ),
                    ),
                  ),
                  const SizedBox(height: 12),

                  // Kota Cuaca HUD (Seksi 2.2)
                  TextField(
                    controller: _cityController,
                    style: const TextStyle(color: BmoTheme.textPrimary),
                    decoration: InputDecoration(
                      labelText: 'Kota Lokasi Cuaca (Seksi 2.2)',
                      hintText: 'Contoh: Jakarta, Surabaya, Bandung',
                      prefixIcon: const Icon(Icons.location_city_rounded, color: BmoTheme.bmoMint, size: 20),
                      labelStyle: const TextStyle(color: BmoTheme.textMuted),
                      filled: true,
                      fillColor: Colors.black.withOpacity(0.25),
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(14),
                        borderSide: const BorderSide(color: BmoTheme.glassBorder),
                      ),
                    ),
                  ),
                  const SizedBox(height: 12),

                  // Dashboard URL
                  TextField(
                    controller: _urlController,
                    style: const TextStyle(color: BmoTheme.textPrimary),
                    decoration: InputDecoration(
                      labelText: 'URL Dashboard Cloud (Opsional)',
                      hintText: 'https://my-bmo.vercel.app',
                      hintStyle: const TextStyle(color: BmoTheme.textMuted, fontSize: 13),
                      prefixIcon: const Icon(Icons.cloud_outlined, color: BmoTheme.bmoMint, size: 20),
                      labelStyle: const TextStyle(color: BmoTheme.textMuted),
                      filled: true,
                      fillColor: Colors.black.withOpacity(0.25),
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(14),
                        borderSide: const BorderSide(color: BmoTheme.glassBorder),
                      ),
                    ),
                  ),
                  const SizedBox(height: 20),

                  // Submit Button
                  ElevatedButton.icon(
                    onPressed: isConnected ? _saveWifiToBmo : null,
                    icon: const Icon(Icons.send_rounded, size: 18),
                    label: const Text('Simpan Wi-Fi & Lokasi ke BMO'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: BmoTheme.primary,
                      foregroundColor: Colors.white,
                      minimumSize: const Size.fromHeight(50),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(14),
                      ),
                    ),
                  ),
                  const SizedBox(height: 10),

                  // Action Buttons Row: Check Status & Disconnect
                  Row(
                    children: [
                      Expanded(
                        child: OutlinedButton.icon(
                          onPressed: isConnected ? _checkWifiStatus : null,
                          icon: const Icon(Icons.refresh, size: 18),
                          label: const Text('Cek Status'),
                          style: OutlinedButton.styleFrom(
                            foregroundColor: BmoTheme.textPrimary,
                            side: const BorderSide(color: BmoTheme.glassBorder),
                            minimumSize: const Size.fromHeight(48),
                            shape: RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(14),
                            ),
                          ),
                        ),
                      ),
                      if (isWifiConnected) ...[
                        const SizedBox(width: 10),
                        Expanded(
                          child: OutlinedButton.icon(
                            onPressed: isConnected ? _disconnectWifi : null,
                            icon: const Icon(Icons.power_settings_new_rounded, size: 18, color: BmoTheme.statusOffline),
                            label: const Text('Putuskan', style: TextStyle(color: BmoTheme.statusOffline)),
                            style: OutlinedButton.styleFrom(
                              side: BorderSide(color: BmoTheme.statusOffline.withOpacity(0.5)),
                              minimumSize: const Size.fromHeight(48),
                              shape: RoundedRectangleBorder(
                                borderRadius: BorderRadius.circular(14),
                              ),
                            ),
                          ),
                        ),
                      ],
                    ],
                  ),
                ],
              ),
            ),
            const SizedBox(height: 16),

            // 4. Spotify Quick Sender Card (Seksi 2.4)
            GlassContainer(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      const Icon(Icons.music_note_rounded, color: BmoTheme.bmoMint, size: 22),
                      const SizedBox(width: 8),
                      const Text(
                        'Spotify "Now Playing" HUD',
                        style: TextStyle(
                          fontSize: 16,
                          fontWeight: FontWeight.w600,
                          color: BmoTheme.textPrimary,
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 6),
                  const Text(
                    'Kirim judul lagu yang sedang Anda dengarkan agar muncul equalizer animasi di layar BMO.',
                    style: TextStyle(fontSize: 12, color: BmoTheme.textSecondary),
                  ),
                  const SizedBox(height: 14),

                  TextField(
                    controller: _spotTitleController,
                    style: const TextStyle(color: BmoTheme.textPrimary),
                    decoration: InputDecoration(
                      labelText: 'Judul Lagu',
                      prefixIcon: const Icon(Icons.audiotrack_rounded, color: BmoTheme.bmoMint, size: 20),
                      labelStyle: const TextStyle(color: BmoTheme.textMuted),
                      filled: true,
                      fillColor: Colors.black.withOpacity(0.25),
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(14),
                        borderSide: const BorderSide(color: BmoTheme.glassBorder),
                      ),
                    ),
                  ),
                  const SizedBox(height: 10),

                  TextField(
                    controller: _spotArtistController,
                    style: const TextStyle(color: BmoTheme.textPrimary),
                    decoration: InputDecoration(
                      labelText: 'Nama Artis / Penyanyi',
                      prefixIcon: const Icon(Icons.person_outline_rounded, color: BmoTheme.bmoMint, size: 20),
                      labelStyle: const TextStyle(color: BmoTheme.textMuted),
                      filled: true,
                      fillColor: Colors.black.withOpacity(0.25),
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(14),
                        borderSide: const BorderSide(color: BmoTheme.glassBorder),
                      ),
                    ),
                  ),
                  const SizedBox(height: 14),

                  ElevatedButton.icon(
                    onPressed: isConnected ? _sendSpotifyToBmo : null,
                    icon: const Icon(Icons.play_circle_fill_rounded, size: 20),
                    label: const Text('Tampilkan Lagu di Layar BMO'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: const Color(0xFF1DB954), // Spotify Green
                      foregroundColor: Colors.white,
                      minimumSize: const Size.fromHeight(48),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(14),
                      ),
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 16),

            // Live status feedback
            Center(
              child: Text(
                _statusFeedback,
                style: const TextStyle(
                  fontSize: 13,
                  color: BmoTheme.textSecondary,
                  fontStyle: FontStyle.italic,
                ),
                textAlign: TextAlign.center,
              ),
            ),
          ],
        );
      },
    );
  }
}
