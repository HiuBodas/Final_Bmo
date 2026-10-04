import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:permission_handler/permission_handler.dart';
import '../services/ble_service.dart';
import '../theme/bmo_theme.dart';
import '../widgets/glass_container.dart';

class SettingsTab extends StatefulWidget {
  const SettingsTab({super.key});

  @override
  State<SettingsTab> createState() => _SettingsTabState();
}

class _SettingsTabState extends State<SettingsTab> {
  final BleService _ble = BleService();
  bool _filterBmoOnly = true;

  @override
  void initState() {
    super.initState();
    _checkPermissions();
  }

  void _checkPermissions() async {
    await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.locationWhenInUse,
    ].request();
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
        final scanResults = _ble.scanResults.where((r) {
          if (!_filterBmoOnly) return true;
          final name = r.device.platformName.toUpperCase();
          return name.contains('BMO') || name.contains('ESP32');
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
                              ? 'Mencari BMO di sekitar...'
                              : 'Tekan "Cari Perangkat BMO" untuk mulai mendeteksi.',
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
                        final name = r.device.platformName.isNotEmpty ? r.device.platformName : 'Perangkat BLE';
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
