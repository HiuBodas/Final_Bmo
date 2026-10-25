import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../services/ble_service.dart';
import '../theme/bmo_theme.dart';
import '../widgets/glass_container.dart';

import '../services/notification_service.dart';

class SettingsTab extends StatefulWidget {
  const SettingsTab({super.key});

  @override
  State<SettingsTab> createState() => _SettingsTabState();
}

class _SettingsTabState extends State<SettingsTab> with WidgetsBindingObserver {
  final BleService _ble = BleService();
  final RealtimeNotificationService _notif = RealtimeNotificationService();
  final TextEditingController _nameController = TextEditingController(text: 'BMO');
  bool _filterBmoOnly = true;
  bool _hasNotifPermission = false;
  bool _autoForward = true;
  bool _filterPopular = false;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    _checkPermissions();
    _loadNotifState();
    _loadSavedName();
    _ble.fetchSystemAndBondedDevices(autoAttach: true);
  }

  void _loadSavedName() async {
    final prefs = await SharedPreferences.getInstance();
    if (mounted) {
      setState(() {
        _nameController.text = prefs.getString('bmo_custom_name') ?? 'BMO';
      });
    }
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    _nameController.dispose();
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.resumed) {
      _loadNotifState();
      // Saat pengguna kembali dari Pengaturan Bluetooth HP, segera cek koneksi manual
      _ble.fetchSystemAndBondedDevices(autoAttach: true);
    }
  }

  void _loadNotifState() async {
    final granted = await _notif.isListenerPermissionGranted();
    if (mounted) {
      setState(() {
        _hasNotifPermission = granted;
        _autoForward = _notif.isAutoForwardEnabled;
        _filterPopular = _notif.filterPopularOnly;
      });
    }
  }

  void _showRestrictedSettingsGuide() {
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
            Icon(Icons.info_outline, color: BmoTheme.bmoMint),
            SizedBox(width: 8),
            Expanded(
              child: Text(
                'Izin Dibatasi Android?',
                style: TextStyle(color: BmoTheme.textPrimary, fontSize: 17, fontWeight: FontWeight.bold),
              ),
            ),
          ],
        ),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Pada Android 13/14+, jika tombol Akses Notifikasi berwarna abu-abu (tidak bisa diklik), ikuti langkah berikut:',
              style: TextStyle(fontSize: 13, color: BmoTheme.textSecondary),
            ),
            const SizedBox(height: 12),
            _guideStep('1', 'Tekan "Buka Info Aplikasi" di bawah.'),
            _guideStep('2', 'Tekan tombol titik tiga (⋮) di pojok kanan atas layar HP.'),
            _guideStep('3', 'Pilih "Izinkan setelan terbatas" (Allow restricted settings) & verifikasi PIN/sidik jari.'),
            _guideStep('4', 'Kembali ke sini dan aktifkan kembali tombol Akses Notifikasi.'),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Tutup', style: TextStyle(color: BmoTheme.textMuted)),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(ctx);
              _notif.openAppDetailsSettings();
            },
            style: ElevatedButton.styleFrom(
              backgroundColor: BmoTheme.primary,
              foregroundColor: Colors.white,
            ),
            child: const Text('Buka Info Aplikasi'),
          ),
        ],
      ),
    );
  }

  Widget _guideStep(String num, String text) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 6),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Container(
            padding: const EdgeInsets.all(4),
            decoration: const BoxDecoration(
              color: BmoTheme.glassBorder,
              shape: BoxShape.circle,
            ),
            child: Text(num, style: const TextStyle(fontSize: 11, fontWeight: FontWeight.bold, color: BmoTheme.bmoMint)),
          ),
          const SizedBox(width: 8),
          Expanded(
            child: Text(text, style: const TextStyle(fontSize: 12, color: BmoTheme.textPrimary)),
          ),
        ],
      ),
    );
  }

  void _checkPermissions() async {
    await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.locationWhenInUse,
      Permission.notification,
    ].request();
    _loadNotifState();
  }

  void _toggleScan() {
    HapticFeedback.lightImpact();
    if (_ble.isScanning) {
      _ble.stopScan();
    } else {
      _ble.startScan();
    }
  }

  @override
  Widget build(BuildContext context) {
    return ListenableBuilder(
      listenable: _ble,
      builder: (context, _) {
        final isConnected = _ble.isConnected;
        final connectedDevice = _ble.connectedDevice;
        final systemDevices = _ble.systemDevices.where((d) {
          if (!_filterBmoOnly) return true;
          final name = d.platformName.toUpperCase();
          return name.contains('BMO') || name.contains('ESP32') || name.isEmpty;
        }).toList();

        final scanResults = _ble.scanResults.where((r) {
          if (!_filterBmoOnly) return true;
          final name = (r.device.platformName.isNotEmpty
                  ? r.device.platformName
                  : r.advertisementData.advName)
              .toUpperCase();
          final hasBmoService =
              r.advertisementData.serviceUuids.contains(BleService.serviceUuid);
          return hasBmoService || name.contains('BMO') || name.contains('ESP32');
        }).toList();

        return ListView(
          padding: const EdgeInsets.fromLTRB(20, 16, 20, 100),
          children: [
            // Title
            Text(
              'Pengaturan & Bluetooth',
              style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                    fontWeight: FontWeight.bold,
                    color: BmoTheme.textPrimary,
                  ),
            ),
            const SizedBox(height: 4),
            const Text(
              'Kelola koneksi Bluetooth Low Energy ke BMO.',
              style: TextStyle(fontSize: 14, color: BmoTheme.textSecondary),
            ),
            const SizedBox(height: 20),

            // Card Kustom Nama Layar BMO
            GlassContainer(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Row(
                    children: [
                      Icon(Icons.badge_rounded, color: BmoTheme.bmoMint, size: 22),
                      SizedBox(width: 8),
                      Text(
                        'Kustom Nama Layar BMO',
                        style: TextStyle(
                          fontSize: 16,
                          fontWeight: FontWeight.bold,
                          color: BmoTheme.textPrimary,
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 6),
                  const Text(
                    'Nama ini akan muncul di pojok kiri atas layar OLED BMO berdampingan dengan jam digital di tengah.',
                    style: TextStyle(fontSize: 12, color: BmoTheme.textSecondary),
                  ),
                  const SizedBox(height: 14),
                  Row(
                    children: [
                      Expanded(
                        child: TextField(
                          controller: _nameController,
                          maxLength: 8,
                          style: const TextStyle(color: BmoTheme.textPrimary, fontWeight: FontWeight.bold),
                          decoration: InputDecoration(
                            labelText: 'Nama BMO (Maks 8 Huruf)',
                            hintText: 'BMO, LUNA, dsb.',
                            labelStyle: const TextStyle(color: BmoTheme.textMuted),
                            filled: true,
                            fillColor: Colors.black.withOpacity(0.25),
                            border: OutlineInputBorder(
                              borderRadius: BorderRadius.circular(14),
                              borderSide: const BorderSide(color: BmoTheme.glassBorder),
                            ),
                            counterText: '',
                          ),
                        ),
                      ),
                      const SizedBox(width: 10),
                      ElevatedButton(
                        onPressed: () async {
                          final name = _nameController.text.trim();
                          if (name.isEmpty) return;
                          HapticFeedback.lightImpact();
                          final prefs = await SharedPreferences.getInstance();
                          await prefs.setString('bmo_custom_name', name);
                          if (_ble.isConnected) {
                            await _ble.sendCustomName(name);
                          }
                          if (mounted) {
                            ScaffoldMessenger.of(context).showSnackBar(
                              SnackBar(content: Text('Nama BMO di layar diubah ke "$name"!')),
                            );
                          }
                        },
                        style: ElevatedButton.styleFrom(
                          backgroundColor: BmoTheme.bmoMint,
                          foregroundColor: Colors.black,
                          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 14),
                          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
                        ),
                        child: const Text('Simpan', style: TextStyle(fontWeight: FontWeight.bold)),
                      ),
                    ],
                  ),
                ],
              ),
            ),
            const SizedBox(height: 16),

            // Active Device Panel
            GlassContainer(
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
                        isConnected ? 'TERHUBUNG' : 'TIDAK TERHUBUNG',
                        style: TextStyle(
                          fontSize: 12,
                          letterSpacing: 1.0,
                          fontWeight: FontWeight.bold,
                          color: isConnected ? BmoTheme.statusOnline : BmoTheme.textMuted,
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 10),
                  Text(
                    isConnected
                        ? (connectedDevice!.platformName.isNotEmpty ? connectedDevice.platformName : 'BMO')
                        : 'Belum Ada BMO Terpilih',
                    style: const TextStyle(
                      fontSize: 18,
                      fontWeight: FontWeight.bold,
                      color: BmoTheme.textPrimary,
                    ),
                  ),
                  if (isConnected) ...[
                    const SizedBox(height: 2),
                    Text(
                      connectedDevice!.remoteId.str,
                      style: const TextStyle(fontSize: 12, color: BmoTheme.textMuted, fontFamily: 'monospace'),
                    ),
                    const SizedBox(height: 14),
                    OutlinedButton.icon(
                      onPressed: () {
                        HapticFeedback.lightImpact();
                        _ble.disconnect();
                      },
                      icon: const Icon(Icons.bluetooth_disabled, size: 18),
                      label: const Text('Putuskan Koneksi'),
                      style: OutlinedButton.styleFrom(
                        foregroundColor: BmoTheme.statusOffline,
                        side: const BorderSide(color: BmoTheme.statusOffline),
                        minimumSize: const Size.fromHeight(46),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                      ),
                    ),
                  ],
                ],
              ),
            ),
            const SizedBox(height: 16),

            // Card Khusus Jika Ada BMO Terhubung di Pengaturan HP (Manual)
            if (systemDevices.isNotEmpty) ...[
              GlassContainer(
                borderColor: BmoTheme.bmoMint.withOpacity(0.5),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      children: [
                        const Icon(Icons.phone_android, color: BmoTheme.bmoMint, size: 20),
                        const SizedBox(width: 8),
                        const Expanded(
                          child: Text(
                            'Koneksi Bluetooth HP (Manual)',
                            style: TextStyle(
                              fontSize: 15,
                              fontWeight: FontWeight.bold,
                              color: BmoTheme.textPrimary,
                            ),
                          ),
                        ),
                        Container(
                          padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
                          decoration: BoxDecoration(
                            color: BmoTheme.bmoMint.withOpacity(0.18),
                            borderRadius: BorderRadius.circular(6),
                          ),
                          child: const Text(
                            'SISTEM HP',
                            style: TextStyle(fontSize: 10, fontWeight: FontWeight.bold, color: BmoTheme.bmoMint),
                          ),
                        ),
                      ],
                    ),
                    const SizedBox(height: 8),
                    const Text(
                      'BMO terdeteksi sudah terhubung atau dipasangkan lewat Pengaturan Bluetooth HP:',
                      style: TextStyle(fontSize: 13, color: BmoTheme.textSecondary),
                    ),
                    const SizedBox(height: 12),
                    ...systemDevices.map((d) {
                      final name = d.platformName.isNotEmpty ? d.platformName : 'BMO Device';
                      final isThisConnected = connectedDevice?.remoteId == d.remoteId;

                      return Container(
                        margin: const EdgeInsets.only(bottom: 8),
                        padding: const EdgeInsets.all(12),
                        decoration: BoxDecoration(
                          color: BmoTheme.glassFill,
                          borderRadius: BorderRadius.circular(12),
                          border: Border.all(
                            color: isThisConnected ? BmoTheme.statusOnline : BmoTheme.glassBorderSubtle,
                          ),
                        ),
                        child: Row(
                          mainAxisAlignment: MainAxisAlignment.spaceBetween,
                          children: [
                            Expanded(
                              child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  Row(
                                    children: [
                                      Text(
                                        name,
                                        style: const TextStyle(fontWeight: FontWeight.bold, color: BmoTheme.textPrimary, fontSize: 15),
                                      ),
                                      if (isThisConnected) ...[
                                        const SizedBox(width: 8),
                                        const Icon(Icons.check_circle, size: 16, color: BmoTheme.statusOnline),
                                      ],
                                    ],
                                  ),
                                  const SizedBox(height: 2),
                                  Text(
                                    '${d.remoteId.str} · Terhubung di Pengaturan HP',
                                    style: const TextStyle(fontSize: 11, color: BmoTheme.textMuted, fontFamily: 'monospace'),
                                  ),
                                ],
                              ),
                            ),
                            const SizedBox(width: 8),
                            if (isThisConnected)
                              const Chip(
                                label: Text('Aktif', style: TextStyle(fontSize: 11, color: Colors.white)),
                                backgroundColor: BmoTheme.statusOnline,
                              )
                            else
                              ElevatedButton(
                                onPressed: () {
                                  HapticFeedback.lightImpact();
                                  _ble.connect(d);
                                },
                                style: ElevatedButton.styleFrom(
                                  backgroundColor: BmoTheme.bmoMint,
                                  foregroundColor: Colors.black87,
                                  padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
                                  shape: RoundedRectangleBorder(
                                    borderRadius: BorderRadius.circular(10),
                                  ),
                                ),
                                child: const Text('Sinkronkan ke APK', style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold)),
                              ),
                          ],
                        ),
                      );
                    }),
                  ],
                ),
              ),
              const SizedBox(height: 16),
            ],

            // Card Pengaturan Notifikasi Real-Time HP -> ESP32
            GlassContainer(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      const Row(
                        children: [
                          Icon(Icons.notifications_active, color: BmoTheme.bmoMint, size: 20),
                          SizedBox(width: 8),
                          Text(
                            'Notifikasi Real-Time HP',
                            style: TextStyle(
                              fontSize: 16,
                              fontWeight: FontWeight.w600,
                              color: BmoTheme.textPrimary,
                            ),
                          ),
                        ],
                      ),
                      Container(
                        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                        decoration: BoxDecoration(
                          color: _hasNotifPermission
                              ? BmoTheme.statusOnline.withOpacity(0.15)
                              : BmoTheme.statusOffline.withOpacity(0.15),
                          borderRadius: BorderRadius.circular(8),
                        ),
                        child: Text(
                          _hasNotifPermission ? 'IZIN AKTIF' : 'IZIN MATI',
                          style: TextStyle(
                            fontSize: 11,
                            fontWeight: FontWeight.bold,
                            color: _hasNotifPermission ? BmoTheme.statusOnline : BmoTheme.statusOffline,
                          ),
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 8),
                  const Text(
                    'Meneruskan pesan masuk (WhatsApp, Telegram, SMS, Panggilan, dll) secara instan ke layar BMO via BLE.',
                    style: TextStyle(fontSize: 13, color: BmoTheme.textSecondary),
                  ),
                  const SizedBox(height: 12),

                  if (!_hasNotifPermission) ...[
                    ElevatedButton.icon(
                      onPressed: () {
                        HapticFeedback.lightImpact();
                        _notif.openListenerSettings();
                      },
                      icon: const Icon(Icons.security, size: 18),
                      label: const Text('Buka Akses Notifikasi HP'),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: BmoTheme.primary,
                        foregroundColor: Colors.white,
                        minimumSize: const Size.fromHeight(46),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                      ),
                    ),
                    const SizedBox(height: 8),
                    OutlinedButton.icon(
                      onPressed: _showRestrictedSettingsGuide,
                      icon: const Icon(Icons.help_outline, size: 16),
                      label: const Text('Tombol Izin Abu-abu / Dibatasi? Klik Ini'),
                      style: OutlinedButton.styleFrom(
                        foregroundColor: BmoTheme.bmoMint,
                        side: const BorderSide(color: BmoTheme.bmoMint),
                        minimumSize: const Size.fromHeight(40),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(10),
                        ),
                      ),
                    ),
                    const SizedBox(height: 8),
                  ],

                  SwitchListTile(
                    contentPadding: EdgeInsets.zero,
                    title: const Text('Teruskan Otomatis ke BMO', style: TextStyle(fontSize: 14, color: BmoTheme.textPrimary)),
                    subtitle: const Text('Kirim seketika saat HP menerima notifikasi', style: TextStyle(fontSize: 12, color: BmoTheme.textMuted)),
                    value: _autoForward,
                    activeColor: BmoTheme.primary,
                    onChanged: (val) {
                      setState(() => _autoForward = val);
                      _notif.setAutoForward(val);
                    },
                  ),
                  const Divider(color: BmoTheme.glassBorderSubtle, height: 1),
                  SwitchListTile(
                    contentPadding: EdgeInsets.zero,
                    title: const Text('Hanya Aplikasi Populer', style: TextStyle(fontSize: 14, color: BmoTheme.textPrimary)),
                    subtitle: const Text('Hanya WhatsApp, Telegram, IG, SMS & Telp', style: TextStyle(fontSize: 12, color: BmoTheme.textMuted)),
                    value: _filterPopular,
                    activeColor: BmoTheme.primary,
                    onChanged: (val) {
                      setState(() => _filterPopular = val);
                      _notif.setFilterPopularOnly(val);
                    },
                  ),
                ],
              ),
            ),
            const SizedBox(height: 16),

            // Scanner Controls Card
            GlassContainer(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      const Text(
                        'Pindai Perangkat',
                        style: TextStyle(
                          fontSize: 16,
                          fontWeight: FontWeight.w600,
                          color: BmoTheme.textPrimary,
                        ),
                      ),
                      Row(
                        children: [
                          const Text('Hanya BMO', style: TextStyle(fontSize: 12, color: BmoTheme.textSecondary)),
                          Switch(
                            value: _filterBmoOnly,
                            onChanged: (val) {
                              setState(() {
                                _filterBmoOnly = val;
                              });
                            },
                            activeColor: BmoTheme.primary,
                          ),
                        ],
                      ),
                    ],
                  ),
                  const SizedBox(height: 8),
                  ElevatedButton.icon(
                    onPressed: _toggleScan,
                    icon: Icon(_ble.isScanning ? Icons.stop : Icons.refresh, size: 18),
                    label: Text(_ble.isScanning ? 'Hentikan Pencarian' : 'Cari Perangkat BMO'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: _ble.isScanning ? BmoTheme.statusOffline : BmoTheme.primary,
                      foregroundColor: Colors.white,
                      minimumSize: const Size.fromHeight(48),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(14),
                      ),
                    ),
                  ),
                  if (_ble.isScanning) ...[
                    const SizedBox(height: 12),
                    const LinearProgressIndicator(
                      color: BmoTheme.bmoMint,
                      backgroundColor: BmoTheme.glassBorder,
                    ),
                  ],
                  const SizedBox(height: 16),

                  // Discovered Devices List
                  if (scanResults.isEmpty)
                    Center(
                      child: Padding(
                        padding: const EdgeInsets.symmetric(vertical: 24),
                        child: Text(
                          _ble.isScanning
                              ? 'Mencari sinyal siaran BMO di sekitar...'
                              : (systemDevices.isNotEmpty
                                  ? 'BMO sudah terdeteksi dari koneksi manual HP di atas.\nTekan tombol "Sinkronkan ke APK" pada kartu di atas.'
                                  : 'Tekan "Cari Perangkat BMO" untuk mulai mendeteksi.'),
                          style: const TextStyle(fontSize: 13, color: BmoTheme.textMuted),
                          textAlign: TextAlign.center,
                        ),
                      ),
                    )
                  else
                    ListView.separated(
                      shrinkWrap: true,
                      physics: const NeverScrollableScrollPhysics(),
                      itemCount: scanResults.length,
                      separatorBuilder: (_, __) => const Divider(color: BmoTheme.glassBorderSubtle, height: 1),
                      itemBuilder: (context, index) {
                        final r = scanResults[index];
                        final name = r.device.platformName.isNotEmpty
                            ? r.device.platformName
                            : (r.advertisementData.advName.isNotEmpty
                                ? r.advertisementData.advName
                                : 'Perangkat BLE');
                        final isThisConnected = connectedDevice?.remoteId == r.device.remoteId;

                        return ListTile(
                          contentPadding: EdgeInsets.zero,
                          leading: const Icon(Icons.bluetooth, color: BmoTheme.bmoMint),
                          title: Text(
                            name,
                            style: const TextStyle(
                              fontSize: 15,
                              fontWeight: FontWeight.w600,
                              color: BmoTheme.textPrimary,
                            ),
                          ),
                          subtitle: Text(
                            'RSSI: ${r.rssi} dBm · ${r.device.remoteId.str}',
                            style: const TextStyle(fontSize: 12, color: BmoTheme.textMuted),
                          ),
                          trailing: isThisConnected
                              ? const Chip(
                                  label: Text('Aktif', style: TextStyle(fontSize: 11, color: Colors.white)),
                                  backgroundColor: BmoTheme.statusOnline,
                                )
                              : ElevatedButton(
                                  onPressed: () {
                                    HapticFeedback.lightImpact();
                                    _ble.connect(r.device);
                                  },
                                  style: ElevatedButton.styleFrom(
                                    backgroundColor: BmoTheme.primary,
                                    foregroundColor: Colors.white,
                                    padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
                                    shape: RoundedRectangleBorder(
                                      borderRadius: BorderRadius.circular(10),
                                    ),
                                  ),
                                  child: const Text('Sambungkan', style: TextStyle(fontSize: 12)),
                                ),
                        );
                      },
                    ),
                ],
              ),
            ),
            const SizedBox(height: 16),

            // Logs Card
            GlassContainer(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      const Text(
                        'Riwayat Log Bluetooth',
                        style: TextStyle(
                          fontSize: 16,
                          fontWeight: FontWeight.w600,
                          color: BmoTheme.textPrimary,
                        ),
                      ),
                      if (_ble.logs.isNotEmpty)
                        TextButton(
                          onPressed: () => _ble.clearLogs(),
                          child: const Text('Bersihkan', style: TextStyle(fontSize: 12, color: BmoTheme.textSecondary)),
                        ),
                    ],
                  ),
                  const SizedBox(height: 8),
                  if (_ble.logs.isEmpty)
                    const Padding(
                      padding: EdgeInsets.symmetric(vertical: 12),
                      child: Text(
                        'Belum ada aktivitas Bluetooth tercatat.',
                        style: TextStyle(fontSize: 13, color: BmoTheme.textMuted),
                      ),
                    )
                  else
                    ListView.builder(
                      shrinkWrap: true,
                      physics: const NeverScrollableScrollPhysics(),
                      itemCount: _ble.logs.length,
                      itemBuilder: (context, index) {
                        final log = _ble.logs[index];
                        return Padding(
                          padding: const EdgeInsets.symmetric(vertical: 4),
                          child: Row(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Text(
                                log.timeFormatted,
                                style: const TextStyle(fontSize: 11, color: BmoTheme.textMuted, fontFamily: 'monospace'),
                              ),
                              const SizedBox(width: 8),
                              Expanded(
                                child: Text(
                                  '${log.title}: ${log.message}',
                                  style: TextStyle(
                                    fontSize: 12,
                                    color: log.isSuccess ? BmoTheme.textPrimary : BmoTheme.statusOffline,
                                  ),
                                ),
                              ),
                            ],
                          ),
                        );
                      },
                    ),
                ],
              ),
            ),
          ],
        );
      },
    );
  }
}
