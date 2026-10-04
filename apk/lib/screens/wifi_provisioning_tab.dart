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
  final TextEditingController _urlController = TextEditingController();

  bool _obscurePassword = true;
  String _statusFeedback = 'Siap mengirim pengaturan Wi-Fi ke BMO.';

  @override
  void initState() {
    super.initState();
    _loadSavedForm();
  }

  void _loadSavedForm() async {
    final prefs = await SharedPreferences.getInstance();
    setState(() {
      _ssidController.text = prefs.getString('bmo_saved_ssid') ?? '';
      _urlController.text = prefs.getString('bmo_saved_url') ?? '';
    });
  }

  @override
  void dispose() {
    _ssidController.dispose();
    _passController.dispose();
    _urlController.dispose();
    super.dispose();
  }

  void _saveWifiToBmo() async {
    final ssid = _ssidController.text.trim();
    final pass = _passController.text.trim();
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
      // Simpan lokal agar tidak perlu mengetik ulang
      final prefs = await SharedPreferences.getInstance();
      await prefs.setString('bmo_saved_ssid', ssid);
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
    _ble.sendCommand(
      '[WIFI] STATUS',
      logTitle: 'Cek Wi-Fi',
      logDetail: 'Permintaan status koneksi',
    );
  }

  @override
  Widget build(BuildContext context) {
    return ListenableBuilder(
      listenable: _ble,
      builder: (context, _) {
        final isConnected = _ble.isConnected;

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
              'Kirim kredensial Wi-Fi langsung via Bluetooth tanpa ribet buka Captive Portal.',
              style: TextStyle(fontSize: 14, color: BmoTheme.textSecondary),
            ),
            const SizedBox(height: 20),

            // Info Card
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

            // Form Glass Card
            GlassContainer(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'Kredensial Jaringan Rumah',
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
                    label: const Text('Simpan Wi-Fi ke BMO'),
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

                  // Check Status Button
                  OutlinedButton.icon(
                    onPressed: isConnected ? _checkWifiStatus : null,
                    icon: const Icon(Icons.refresh, size: 18),
                    label: const Text('Cek Status & IP BMO'),
                    style: OutlinedButton.styleFrom(
                      foregroundColor: BmoTheme.textPrimary,
                      side: const BorderSide(color: BmoTheme.glassBorder),
                      minimumSize: const Size.fromHeight(50),
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
