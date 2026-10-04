import 'dart:async';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../models/bmo_log.dart';

class BleService extends ChangeNotifier {
  static final BleService _instance = BleService._internal();
  factory BleService() => _instance;
  BleService._internal();

  // Nordic UART Service UUIDs
  static final Guid serviceUuid = Guid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
  static final Guid rxUuid = Guid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"); // HP -> ESP32
  static final Guid txUuid = Guid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E"); // ESP32 -> HP

  BluetoothDevice? _connectedDevice;
  BluetoothCharacteristic? _rxCharacteristic;
  BluetoothCharacteristic? _txCharacteristic;

  bool _isScanning = false;
  List<ScanResult> _scanResults = [];
  final List<BmoLogEntry> _logs = [];
  StreamSubscription? _scanSub;
  StreamSubscription? _connectionSub;
  StreamSubscription? _notifySub;

  BluetoothDevice? get connectedDevice => _connectedDevice;
  bool get isConnected => _connectedDevice != null;
  bool get isScanning => _isScanning;
  List<ScanResult> get scanResults => _scanResults;
  List<BmoLogEntry> get logs => List.unmodifiable(_logs);

  Future<void> init() async {
    FlutterBluePlus.isScanning.listen((scanning) {
      _isScanning = scanning;
      notifyListeners();
    });

    final prefs = await SharedPreferences.getInstance();
    final savedId = prefs.getString('saved_bmo_device_id');
    if (savedId != null && savedId.isNotEmpty) {
      // Coba re-connect otomatis jika bluetooth aktif
      _tryAutoConnect(savedId);
    }
  }

  void _tryAutoConnect(String remoteId) async {
    try {
      final device = BluetoothDevice.fromId(remoteId);
      await connect(device);
    } catch (e) {
      debugPrint('[BLE] Auto-connect error: $e');
    }
  }

  Future<void> startScan({Duration timeout = const Duration(seconds: 12)}) async {
    if (_isScanning) return;
    _scanResults = [];
    notifyListeners();

    try {
      await FlutterBluePlus.startScan(
        timeout: timeout,
        androidUsesFineLocation: true,
      );

      _scanSub?.cancel();
      _scanSub = FlutterBluePlus.scanResults.listen((results) {
        _scanResults = results;
        notifyListeners();
      });
    } catch (e) {
      debugPrint('[BLE] Scan error: $e');
    }
  }

  Future<void> stopScan() async {
    await FlutterBluePlus.stopScan();
    _isScanning = false;
    notifyListeners();
  }

  Future<bool> connect(BluetoothDevice device) async {
    try {
      await stopScan();
      await device.connect(timeout: const Duration(seconds: 10), autoConnect: false);

      _connectedDevice = device;
      notifyListeners();

      // Negosiasi MTU 512 agar notifikasi panjang tidak terpotong (default hanya 20 byte)
      try {
        await device.requestMtu(512);
        debugPrint('[BLE] Negosiasi MTU 512 berhasil.');
      } catch (e) {
        debugPrint('[BLE] Request MTU notice: $e');
      }

      // Temukan service & karakteristik
      final services = await device.discoverServices();
      for (final s in services) {
        if (s.uuid == serviceUuid) {
          for (final c in s.characteristics) {
            if (c.uuid == rxUuid) {
              _rxCharacteristic = c;
            } else if (c.uuid == txUuid) {
              _txCharacteristic = c;
              await c.setNotifyValue(true);
              _notifySub?.cancel();
              _notifySub = c.onValueReceived.listen((bytes) {
                final message = utf8.decode(bytes, allowMalformed: true);
                _handleIncomingDeviceMessage(message);
              });
            }
          }
        }
      }

      // Simpan device ID
      final prefs = await SharedPreferences.getInstance();
      await prefs.setString('saved_bmo_device_id', device.remoteId.str);

      _addLog('Terhubung ke BMO', device.platformName.isNotEmpty ? device.platformName : 'BMO Device', true);

      // Pantau jika terputus
      _connectionSub?.cancel();
      _connectionSub = device.connectionState.listen((state) {
        if (state == BluetoothConnectionState.disconnected) {
          _connectedDevice = null;
          _rxCharacteristic = null;
          _txCharacteristic = null;
          _addLog('Koneksi Terputus', 'BMO offline atau di luar jangkauan', false);
          notifyListeners();
        }
      });

      return true;
    } catch (e) {
      debugPrint('[BLE] Connection error: $e');
      _addLog('Gagal Terhubung', e.toString(), false);
      _connectedDevice = null;
      notifyListeners();
      return false;
    }
  }

  Future<void> disconnect() async {
    if (_connectedDevice != null) {
      await _connectedDevice!.disconnect();
      _connectedDevice = null;
      _rxCharacteristic = null;
      _txCharacteristic = null;
      notifyListeners();
    }
  }

  Future<bool> sendCommand(String payload, {String? logTitle, String? logDetail}) async {
    if (_rxCharacteristic == null) {
      _addLog('Gagal Mengirim', 'BMO belum terhubung via Bluetooth', false);
      return false;
    }

    try {
      final bytes = utf8.encode(payload);
      
      // Pilih mode write yang didukung karakteristik:
      final bool canWriteWithResp = _rxCharacteristic!.properties.write;
      final bool canWriteNoResp = _rxCharacteristic!.properties.writeWithoutResponse;
      
      bool useWithoutResp = false;
      if (!canWriteWithResp && canWriteNoResp) {
        useWithoutResp = true;
      }

      try {
        await _rxCharacteristic!.write(bytes, withoutResponse: useWithoutResp);
      } catch (primaryErr) {
        debugPrint('[BLE] Primary write failed ($primaryErr), mencoba mode alternatif...');
        // Coba alternatif (kebalikan) jika gagal
        await _rxCharacteristic!.write(bytes, withoutResponse: !useWithoutResp);
      }

      _addLog(
        logTitle ?? 'Perintah Terkirim',
        logDetail ?? payload,
        true,
      );
      return true;
    } catch (e) {
      debugPrint('[BLE] Write error: $e');
      _addLog('Gagal Mengirim', e.toString(), false);
      return false;
    }
  }

  void _handleIncomingDeviceMessage(String msg) {
    debugPrint('[BLE RX] Dari BMO: $msg');
    _addLog('Pesan dari BMO', msg, true);
  }

  void _addLog(String title, String message, bool isSuccess) {
    _logs.insert(0, BmoLogEntry(title: title, message: message, isSuccess: isSuccess));
    if (_logs.length > 50) _logs.removeLast();
    notifyListeners();
  }

  void clearLogs() {
    _logs.clear();
    notifyListeners();
  }
}
