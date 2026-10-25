import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:intl/intl.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../services/ble_service.dart';
import '../services/notification_service.dart';
import '../theme/bmo_theme.dart';
import '../widgets/glass_container.dart';
import '../widgets/bmo_face_avatar.dart';

class HomeTab extends StatefulWidget {
  final VoidCallback onNavigateToSettings;
  final VoidCallback onNavigateToSimulator;
  final VoidCallback onNavigateToWifi;
  final VoidCallback? onNavigateToDoodle;

  const HomeTab({
    super.key,
    required this.onNavigateToSettings,
    required this.onNavigateToSimulator,
    required this.onNavigateToWifi,
    this.onNavigateToDoodle,
  });

  @override
  State<HomeTab> createState() => _HomeTabState();
}

class _HomeTabState extends State<HomeTab> with WidgetsBindingObserver {
  final BleService _ble = BleService();
  final RealtimeNotificationService _notif = RealtimeNotificationService();
  final TextEditingController _msgController = TextEditingController();
  final TextEditingController _senderController = TextEditingController(text: 'Luna');
  bool _hasNotifPermission = false;
  String _customBmoName = 'BMO';

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    _checkNotifPermission();
    _loadCustomName();
    _ble.fetchSystemAndBondedDevices(autoAttach: true);
  }

  void _loadCustomName() async {
    final prefs = await SharedPreferences.getInstance();
    if (mounted) {
      setState(() {
        _customBmoName = prefs.getString('bmo_custom_name') ?? 'BMO';
      });
    }
  }

  void _showCustomNameDialog() {
    final controller = TextEditingController(text: _customBmoName);
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        backgroundColor: BmoTheme.surfaceElevated,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(20),
          side: const BorderSide(color: BmoTheme.glassBorder),
        ),
        title: const Row(
          children: [
            Icon(Icons.badge_rounded, color: BmoTheme.bmoMint),
            SizedBox(width: 8),
            Text(
              'Kustom Nama BMO',
              style: TextStyle(color: BmoTheme.textPrimary, fontSize: 18, fontWeight: FontWeight.bold),
            ),
          ],
        ),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Ubah nama yang muncul di pojok kiri atas layar OLED BMO (berdampingan dengan jam digital di tengah):',
              style: TextStyle(fontSize: 13, color: BmoTheme.textSecondary),
            ),
            const SizedBox(height: 14),
            TextField(
              controller: controller,
              maxLength: 8,
              autofocus: true,
              style: const TextStyle(color: BmoTheme.textPrimary, fontWeight: FontWeight.bold),
              decoration: InputDecoration(
                labelText: 'Nama Layar BMO (Maks 8 Huruf)',
                hintText: 'Contoh: BMO, LUNA',
                labelStyle: const TextStyle(color: BmoTheme.textMuted),
                filled: true,
                fillColor: Colors.black.withOpacity(0.3),
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(14),
                  borderSide: const BorderSide(color: BmoTheme.glassBorder),
                ),
                counterStyle: const TextStyle(color: BmoTheme.textMuted),
              ),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Batal', style: TextStyle(color: BmoTheme.textMuted)),
          ),
          ElevatedButton(
            onPressed: () async {
              final newName = controller.text.trim();
              if (newName.isEmpty) return;
              Navigator.pop(ctx);

              setState(() {
                _customBmoName = newName;
              });

              final prefs = await SharedPreferences.getInstance();
              await prefs.setString('bmo_custom_name', newName);

              if (_ble.isConnected) {
                await _ble.sendCustomName(newName);
              }

              if (mounted) {
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(content: Text('Nama layar BMO diubah menjadi "$newName"!')),
                );
              }
            },
            style: ElevatedButton.styleFrom(
              backgroundColor: BmoTheme.bmoMint,
              foregroundColor: Colors.black,
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
            ),
            child: const Text('Simpan & Terapkan', style: TextStyle(fontWeight: FontWeight.bold)),
          ),
        ],
      ),
    );
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.resumed) {
      _checkNotifPermission();
      _ble.fetchSystemAndBondedDevices(autoAttach: true);
    }
  }

  void _checkNotifPermission() async {
    final granted = await _notif.isListenerPermissionGranted();
    if (mounted) {
      setState(() => _hasNotifPermission = granted);
    }
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    _msgController.dispose();
    _senderController.dispose();
    super.dispose();
  }

  void _syncTime() {
    HapticFeedback.lightImpact();
    final now = DateTime.now();
    final timeStr = DateFormat('HH:mm:ss').format(now);
    final dateStr = DateFormat('dd/MM/yyyy').format(now);
    final dayStr = DateFormat('E').format(now);
    // Format payload kompatibel: [TIME] HH:mm:ss|dd/MM/yyyy|Day|100%
    final payload = '[TIME] $timeStr|$dateStr|$dayStr|100%';

    _ble.sendCommand(
      payload,
      logTitle: 'Sinkronisasi Jam',
      logDetail: '$timeStr · $dateStr',
    );
  }

  void _sendQuickMessage() {
    final text = _msgController.text.trim();
    final sender = _senderController.text.trim().isEmpty ? 'HP' : _senderController.text.trim();
    if (text.isEmpty) return;

    HapticFeedback.lightImpact();
    final payload = '[PESAN] $sender: $text';
    _ble.sendCommand(
      payload,
      logTitle: 'Pesan Terkirim',
      logDetail: '$sender: $text',
    );
    _msgController.clear();
  }

  @override
  Widget build(BuildContext context) {
    return ListenableBuilder(
      listenable: _ble,
      builder: (context, _) {
        final isConnected = _ble.isConnected;
        final deviceName = _ble.connectedDevice?.platformName.isNotEmpty == true
            ? _ble.connectedDevice!.platformName
            : 'BMO';

        return ListView(
          padding: const EdgeInsets.fromLTRB(20, 16, 20, 100),
          children: [
            // Header
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              crossAxisAlignment: CrossAxisAlignment.center,
              children: [
                Text(
                  'BMO Companion',
                  style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                        fontWeight: FontWeight.bold,
                        color: BmoTheme.textPrimary,
                      ),
                ),
                GestureDetector(
                  onTap: widget.onNavigateToSimulator,
                  child: const BmoFaceAvatar(
                    mood: 'happy',
                    width: 56,
                    height: 42,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 20),

            // Hero Connection Card
            GlassContainer(
              fill: isConnected ? const Color(0x2210B981) : BmoTheme.glassFill,
              borderColor: isConnected ? BmoTheme.statusOnline.withOpacity(0.4) : BmoTheme.glassBorder,
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      Container(
                        width: 10,
                        height: 10,
                        decoration: BoxDecoration(
                          color: isConnected ? BmoTheme.statusOnline : BmoTheme.statusOffline,
                          shape: BoxShape.circle,
                        ),
                      ),
                      const SizedBox(width: 8),
                      Text(
                        isConnected ? 'BMO TERHUBUNG' : 'BMO BELUM TERHUBUNG',
                        style: TextStyle(
                          fontSize: 12,
                          letterSpacing: 1.0,
                          fontWeight: FontWeight.w600,
                          color: isConnected ? BmoTheme.statusOnline : BmoTheme.textMuted,
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 12),
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      Expanded(
                        child: Text(
                          isConnected ? deviceName : 'Siap Menemani Harimu',
                          style: const TextStyle(
                            fontSize: 20,
                            fontWeight: FontWeight.bold,
                            color: BmoTheme.textPrimary,
                          ),
                          overflow: TextOverflow.ellipsis,
                        ),
                      ),
                      InkWell(
                        onTap: _showCustomNameDialog,
                        borderRadius: BorderRadius.circular(10),
                        child: Container(
                          padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                          decoration: BoxDecoration(
                            color: BmoTheme.primary.withOpacity(0.2),
                            borderRadius: BorderRadius.circular(10),
                            border: Border.all(color: BmoTheme.primaryLight.withOpacity(0.4)),
                          ),
                          child: Row(
                            mainAxisSize: MainAxisSize.min,
                            children: [
                              const Icon(Icons.edit_rounded, size: 13, color: BmoTheme.bmoMint),
                              const SizedBox(width: 4),
                              Text(
                                _customBmoName,
                                style: const TextStyle(
                                  fontSize: 12,
                                  fontWeight: FontWeight.bold,
                                  color: BmoTheme.bmoMint,
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 6),
                  Text(
                    isConnected
                        ? 'Bluetooth Low Energy aktif. BMO siap menerima perintah ekspresi dan data.'
                        : (_ble.systemDevices.isNotEmpty
                            ? 'BMO terdeteksi sudah terhubung di Bluetooth HP! Tekan tombol di bawah untuk menyinkronkan ke aplikasi.'
                            : 'Nyalakan BMO dan hubungkan via Bluetooth untuk mulai berinteraksi.'),
                    style: const TextStyle(
                      fontSize: 13,
                      height: 1.4,
                      color: BmoTheme.textSecondary,
                    ),
                  ),
                  const SizedBox(height: 16),
                  if (!isConnected && _ble.systemDevices.isNotEmpty) ...[
                    ElevatedButton.icon(
                      onPressed: () {
                        HapticFeedback.lightImpact();
                        _ble.connect(_ble.systemDevices.first);
                      },
                      icon: const Icon(Icons.sync, size: 18),
                      label: const Text('Sinkronkan BMO dari HP'),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: BmoTheme.bmoMint,
                        foregroundColor: Colors.black87,
                        elevation: 0,
                        minimumSize: const Size.fromHeight(48),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(14),
                        ),
                      ),
                    ),
                    const SizedBox(height: 8),
                  ],
                  ElevatedButton.icon(
                    onPressed: widget.onNavigateToSettings,
                    icon: Icon(
                      isConnected ? Icons.settings_bluetooth : Icons.search,
                      size: 18,
                    ),
                    label: Text(isConnected ? 'Kelola Perangkat' : 'Cari BMO Sekarang'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: isConnected ? BmoTheme.primary : BmoTheme.surfaceElevated,
                      foregroundColor: Colors.white,
                      elevation: 0,
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

            // Quick Banner Status Wi-Fi (Jika BLE Terhubung)
            if (isConnected) ...[
              GestureDetector(
                onTap: widget.onNavigateToWifi,
                child: GlassContainer(
                  fill: _ble.wifiState == 'CONNECTED'
                      ? const Color(0x1810B981)
                      : (_ble.wifiState == 'CONNECTING'
                          ? const Color(0x18F59E0B)
                          : const Color(0x153B82F6)),
                  borderColor: _ble.wifiState == 'CONNECTED'
                      ? BmoTheme.statusOnline.withOpacity(0.35)
                      : (_ble.wifiState == 'CONNECTING'
                          ? const Color(0x55F59E0B)
                          : BmoTheme.primaryLight.withOpacity(0.3)),
                  child: Row(
                    children: [
                      Icon(
                        _ble.wifiState == 'CONNECTED'
                            ? Icons.wifi_rounded
                            : (_ble.wifiState == 'CONNECTING'
                                ? Icons.wifi_find_rounded
                                : Icons.wifi_off_rounded),
                        color: _ble.wifiState == 'CONNECTED'
                            ? BmoTheme.statusOnline
                            : (_ble.wifiState == 'CONNECTING'
                                ? const Color(0xFFF59E0B)
                                : BmoTheme.bmoMint),
                        size: 24,
                      ),
                      const SizedBox(width: 12),
                      Expanded(
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Text(
                              _ble.wifiState == 'CONNECTED'
                                  ? 'Wi-Fi BMO Terhubung'
                                  : (_ble.wifiState == 'CONNECTING'
                                      ? 'Menyambung ke Wi-Fi...'
                                      : 'Wi-Fi BMO Belum Aktif'),
                              style: const TextStyle(
                                fontSize: 14,
                                fontWeight: FontWeight.bold,
                                color: BmoTheme.textPrimary,
                              ),
                            ),
                            const SizedBox(height: 2),
                            Text(
                              _ble.wifiState == 'CONNECTED'
                                  ? (_ble.wifiIp != null
                                      ? 'IP: ${_ble.wifiIp} · Web: http://bmo.local'
                                      : 'Web Dashboard: http://bmo.local')
                                  : 'Ketuk untuk hubungkan Wi-Fi, NTP & Cuaca',
                              style: const TextStyle(fontSize: 12, color: BmoTheme.textSecondary),
                              maxLines: 1,
                              overflow: TextOverflow.ellipsis,
                            ),
                          ],
                        ),
                      ),
                      const Icon(Icons.chevron_right, color: BmoTheme.textMuted, size: 20),
                    ],
                  ),
                ),
              ),
              const SizedBox(height: 16),
            ],

            // Banner Status Notifikasi Real-time
            GestureDetector(
              onTap: widget.onNavigateToSettings,
              child: GlassContainer(
                fill: _hasNotifPermission ? const Color(0x1510B981) : const Color(0x15F59E0B),
                borderColor: _hasNotifPermission ? BmoTheme.statusOnline.withOpacity(0.3) : const Color(0x55F59E0B),
                child: Row(
                  children: [
                    Icon(
                      _hasNotifPermission ? Icons.notifications_active : Icons.notification_important,
                      color: _hasNotifPermission ? BmoTheme.bmoMint : const Color(0xFFF59E0B),
                      size: 24,
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            _hasNotifPermission ? 'Notifikasi Real-Time Aktif' : 'Akses Notifikasi Belum Aktif',
                            style: const TextStyle(
                              fontSize: 14,
                              fontWeight: FontWeight.bold,
                              color: BmoTheme.textPrimary,
                            ),
                          ),
                          const SizedBox(height: 2),
                          Text(
                            _hasNotifPermission
                                ? 'WhatsApp, IG, Telp & SMS diteruskan instan ke BMO'
                                : 'Klik untuk mengaktifkan izin agar BMO menerima notifikasi HP',
                            style: const TextStyle(fontSize: 12, color: BmoTheme.textSecondary),
                          ),
                        ],
                      ),
                    ),
                    const Icon(Icons.chevron_right, color: BmoTheme.textMuted, size: 20),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 16),

            // Card Kanvas Gambar Live
            if (widget.onNavigateToDoodle != null) ...[
              GlassContainer(
                fill: const Color(0x1C3B82F6),
                borderColor: BmoTheme.primaryLight.withOpacity(0.4),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Row(
                      children: [
                        Icon(Icons.draw_rounded, color: BmoTheme.bmoMint, size: 22),
                        SizedBox(width: 8),
                        Text(
                          'Kanvas Gambar BMO (Live Doodle)',
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
                      'Gambar bebas dengan jarimu di kanvas HP dan langsung tampil otomatis di layar OLED BMO!',
                      style: TextStyle(fontSize: 13, color: BmoTheme.textSecondary),
                    ),
                    const SizedBox(height: 14),
                    ElevatedButton.icon(
                      onPressed: widget.onNavigateToDoodle,
                      icon: const Icon(Icons.palette_rounded, size: 18),
                      label: const Text('Buka Kanvas Gambar Sekarang'),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: BmoTheme.primary,
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
            ],

            // Quick Sync Card
            GlassContainer(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'Sinkronisasi Jam & Waktu',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.w600,
                      color: BmoTheme.textPrimary,
                    ),
                  ),
                  const SizedBox(height: 6),
                  const Text(
                    'Samakan jam digital BMO dengan waktu akurat dari ponselmu.',
                    style: TextStyle(fontSize: 13, color: BmoTheme.textSecondary),
                  ),
                  const SizedBox(height: 14),
                  OutlinedButton.icon(
                    onPressed: isConnected ? _syncTime : null,
                    icon: const Icon(Icons.sync, size: 18),
                    label: const Text('Sinkronkan Jam Sekarang'),
                    style: OutlinedButton.styleFrom(
                      foregroundColor: BmoTheme.bmoMint,
                      side: const BorderSide(color: BmoTheme.bmoMint),
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

            // Quick Message Card
            GlassContainer(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'Kirim Pesan ke Layar BMO',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.w600,
                      color: BmoTheme.textPrimary,
                    ),
                  ),
                  const SizedBox(height: 12),
                  TextField(
                    controller: _senderController,
                    style: const TextStyle(color: BmoTheme.textPrimary, fontSize: 14),
                    decoration: InputDecoration(
                      labelText: 'Pengirim',
                      labelStyle: const TextStyle(color: BmoTheme.textMuted),
                      filled: true,
                      fillColor: Colors.black.withOpacity(0.2),
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(12),
                        borderSide: const BorderSide(color: BmoTheme.glassBorder),
                      ),
                    ),
                  ),
                  const SizedBox(height: 10),
                  TextField(
                    controller: _msgController,
                    style: const TextStyle(color: BmoTheme.textPrimary, fontSize: 14),
                    decoration: InputDecoration(
                      labelText: 'Tulis pesan...',
                      labelStyle: const TextStyle(color: BmoTheme.textMuted),
                      filled: true,
                      fillColor: Colors.black.withOpacity(0.2),
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(12),
                        borderSide: const BorderSide(color: BmoTheme.glassBorder),
                      ),
                    ),
                  ),
                  const SizedBox(height: 12),
                  ElevatedButton.icon(
                    onPressed: isConnected ? _sendQuickMessage : null,
                    icon: const Icon(Icons.send_rounded, size: 18),
                    label: const Text('Tampilkan di Layar BMO'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: BmoTheme.primary,
                      foregroundColor: Colors.white,
                      minimumSize: const Size.fromHeight(48),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(14),
                      ),
                    ),
                  ),
                  const SizedBox(height: 14),
                  const Text(
                    'Uji Coba Notifikasi Cepat:',
                    style: TextStyle(
                      fontSize: 12,
                      fontWeight: FontWeight.w600,
                      color: BmoTheme.textSecondary,
                    ),
                  ),
                  const SizedBox(height: 8),
                  SingleChildScrollView(
                    scrollDirection: Axis.horizontal,
                    child: Row(
                      children: [
                        _buildQuickNotifChip(
                          label: 'WhatsApp',
                          color: const Color(0xFF25D366),
                          payload: '[WHATSAPP] Budi: Halo! Jadi meeting siang ini?',
                          isConnected: isConnected,
                        ),
                        const SizedBox(width: 8),
                        _buildQuickNotifChip(
                          label: 'Instagram',
                          color: const Color(0xFFE1306C),
                          payload: '[INSTAGRAM] Salsa: Menyukai postinganmu',
                          isConnected: isConnected,
                        ),
                        const SizedBox(width: 8),
                        _buildQuickNotifChip(
                          label: 'SMS / OTP',
                          color: const Color(0xFF3B82F6),
                          payload: '[SMS] Bank: Kode OTP Anda 739182',
                          isConnected: isConnected,
                        ),
                        const SizedBox(width: 8),
                        _buildQuickNotifChip(
                          label: 'Panggilan',
                          color: const Color(0xFF10B981),
                          payload: '[PANGGILAN] Ibu: Sedang memanggil...',
                          isConnected: isConnected,
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 16),

            // Activity Logs Preview
            if (_ble.logs.isNotEmpty) ...[
              const Text(
                'Aktivitas Terbaru',
                style: TextStyle(
                  fontSize: 15,
                  fontWeight: FontWeight.w600,
                  color: BmoTheme.textPrimary,
                ),
              ),
              const SizedBox(height: 8),
              GlassContainer(
                padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                child: Column(
                  children: _ble.logs.take(3).map((log) {
                    return Padding(
                      padding: const EdgeInsets.symmetric(vertical: 6),
                      child: Row(
                        children: [
                          Icon(
                            log.isSuccess ? Icons.check_circle_outline : Icons.error_outline,
                            size: 16,
                            color: log.isSuccess ? BmoTheme.statusOnline : BmoTheme.statusOffline,
                          ),
                          const SizedBox(width: 8),
                          Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Text(
                                  log.title,
                                  style: const TextStyle(
                                    fontSize: 13,
                                    fontWeight: FontWeight.w600,
                                    color: BmoTheme.textPrimary,
                                  ),
                                ),
                                Text(
                                  log.message,
                                  style: const TextStyle(
                                    fontSize: 12,
                                    color: BmoTheme.textSecondary,
                                  ),
                                  maxLines: 1,
                                  overflow: TextOverflow.ellipsis,
                                ),
                              ],
                            ),
                          ),
                          Text(
                            log.timeFormatted,
                            style: const TextStyle(fontSize: 11, color: BmoTheme.textMuted),
                          ),
                        ],
                      ),
                    );
                  }).toList(),
                ),
              ),
            ],
          ],
        );
      },
    );
  }

  Widget _buildQuickNotifChip({
    required String label,
    required Color color,
    required String payload,
    required bool isConnected,
  }) {
    return ActionChip(
      avatar: CircleAvatar(backgroundColor: color, radius: 4),
      label: Text(label, style: const TextStyle(fontSize: 12, color: BmoTheme.textPrimary)),
      backgroundColor: Colors.white.withOpacity(0.06),
      side: BorderSide(color: Colors.white.withOpacity(0.15)),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
      onPressed: isConnected
          ? () {
              HapticFeedback.lightImpact();
              _ble.sendCommand(
                payload,
                logTitle: 'Uji Notifikasi ($label)',
                logDetail: payload,
              );
            }
          : null,
    );
  }
}
