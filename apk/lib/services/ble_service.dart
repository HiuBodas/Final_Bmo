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

  List<BluetoothDevice> _systemDevices = [];

  String _wifiState = 'UNKNOWN'; // 'CONNECTED', 'CONNECTING', 'DISCONNECTED', 'FAILED', 'OFF', 'UNKNOWN'
  String? _wifiIp;
  String? _wifiSsid;

  BluetoothDevice? get connectedDevice => _connectedDevice;
  bool get isConnected => _connectedDevice != null;
  bool get isScanning => _isScanning;
  List<ScanResult> get scanResults => _scanResults;
  List<BluetoothDevice> get systemDevices => _systemDevices;
  List<BmoLogEntry> get logs => List.unmodifiable(_logs);
  String get wifiState => _wifiState;
  String? get wifiIp => _wifiIp;
  String? get wifiSsid => _wifiSsid;

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

    // Periksa juga apakah user sudah menghubungkan BMO secara manual di Pengaturan HP
    await fetchSystemAndBondedDevices(autoAttach: true);
  }

  void _tryAutoConnect(String remoteId) async {
    try {
      final device = BluetoothDevice.fromId(remoteId);
      await connect(device);
    } catch (e) {
      debugPrint('[BLE] Auto-connect error: $e');
    }
  }

  /// Memindai perangkat BMO yang sudah terhubung di level OS/Android atau di-pair manual di Bluetooth HP
  Future<void> fetchSystemAndBondedDevices({bool autoAttach = true}) async {
    try {
      final state = await FlutterBluePlus.adapterState.first;
      if (state != BluetoothAdapterState.on) {
        return;
      }
    } catch (_) {}

    final List<BluetoothDevice> detected = [];
    final Set<String> seenIds = {};

    void addDevice(BluetoothDevice d) {
      if (!seenIds.contains(d.remoteId.str)) {
        seenIds.add(d.remoteId.str);
        detected.add(d);
      }
    }

    // 1. Cek perangkat yang sudah terhubung di level OS/Android (systemDevices dengan Nordic UART UUID)
    try {
      final sysWithUuid = await FlutterBluePlus.systemDevices([serviceUuid]);
      for (final d in sysWithUuid) {
        addDevice(d);
      }
    } catch (e) {
      debugPrint('[BLE] systemDevices with UUID notice: $e');
    }

    // 2. Cek semua perangkat yang terhubung di sistem Android (tanpa filter UUID)
    try {
      final allSys = await FlutterBluePlus.systemDevices([]);
      for (final d in allSys) {
        final name = d.platformName.toUpperCase();
        if (name.contains('BMO') || name.contains('ESP32') || name.isEmpty) {
          addDevice(d);
        }
      }
    } catch (e) {
      debugPrint('[BLE] all systemDevices notice: $e');
    }

    // 3. Cek perangkat yang sudah di-pair/bonding manual di Pengaturan Bluetooth HP
    try {
      final bonded = await FlutterBluePlus.bondedDevices;
      for (final d in bonded) {
        final name = d.platformName.toUpperCase();
        if (name.contains('BMO') || name.contains('ESP32') || name.isEmpty) {
          addDevice(d);
        }
      }
    } catch (e) {
      debugPrint('[BLE] bondedDevices notice: $e');
    }

    // 4. Cek connectedDevices dari FlutterBluePlus
    try {
      for (final d in FlutterBluePlus.connectedDevices) {
        final name = d.platformName.toUpperCase();
        if (name.contains('BMO') || name.contains('ESP32') || name.isEmpty) {
          addDevice(d);
        }
      }
    } catch (e) {
      debugPrint('[BLE] connectedDevices notice: $e');
    }

    _systemDevices = detected;
    notifyListeners();

    // Auto-connect jika belum terhubung dan ada BMO yang sudah tersambung di sistem Android HP:
    if (autoAttach && !isConnected && _systemDevices.isNotEmpty) {
      BluetoothDevice? targetBmo;
      for (final d in _systemDevices) {
        final name = d.platformName.toUpperCase();
        if (name.contains('BMO') || name.contains('ESP32')) {
          targetBmo = d;
          break;
        }
      }
      targetBmo ??= _systemDevices.first;

      debugPrint('[BLE] Mendeteksi BMO di sistem Bluetooth HP (${targetBmo.remoteId.str}), menyambungkan ke aplikasi...');
      _addLog('BMO Terdeteksi di HP', 'Menghubungkan otomatis dari Bluetooth HP (${targetBmo.remoteId.str})...', true);
      await connect(targetBmo);
    }
  }

  Future<void> startScan({Duration timeout = const Duration(seconds: 12)}) async {
    if (_isScanning) return;
    _scanResults = [];
    notifyListeners();

    // Periksa terlebih dahulu perangkat yang sudah terhubung secara manual di HP
    await fetchSystemAndBondedDevices(autoAttach: true);

    if (isConnected) {
      // Jika sudah terhubung ke BMO manual, tidak perlu memindai lagi
      return;
    }

    try {
      // Dengarkan hasil scan SEBELUM scan dimulai agar tidak ada hasil terlewat
      _scanSub?.cancel();
      _scanSub = FlutterBluePlus.scanResults.listen((results) {
        _scanResults = results;
        notifyListeners();
      });

      // androidUsesFineLocation harus false: izin ACCESS_FINE_LOCATION hanya
      // dideklarasikan sampai Android 11 (maxSdkVersion 30). Jika true, scan
      // gagal diam-diam di Android 12+ dan BMO tidak pernah terdeteksi.
      await FlutterBluePlus.startScan(
        timeout: timeout,
        androidUsesFineLocation: false,
      );
    } catch (e) {
      debugPrint('[BLE] Scan error: $e');
      _addLog('Gagal Memindai', e.toString(), false);
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

      // Periksa apakah perangkat sudah berstatus connected di level stack BLE
      bool alreadyConnected = false;
      try {
        alreadyConnected = device.isConnected;
      } catch (_) {}

      if (!alreadyConnected) {
        try {
          await device.connect(timeout: const Duration(seconds: 10), autoConnect: false);
        } catch (connErr) {
          debugPrint('[BLE] device.connect info: $connErr');
          // Jika perangkat sudah terhubung di OS Android, connect() bisa melempar pesan already connected
          final errStr = connErr.toString().toLowerCase();
          if (!errStr.contains('already connected') && !errStr.contains('connected')) {
            rethrow;
          }
        }
      }

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

      if (_rxCharacteristic == null) {
        debugPrint('[BLE] Catatan: Karakteristik RX belum terdaftar, mencoba ulang discovery...');
      }

      // Simpan device ID
      final prefs = await SharedPreferences.getInstance();
      await prefs.setString('saved_bmo_device_id', device.remoteId.str);

      _addLog('Terhubung ke BMO', device.platformName.isNotEmpty ? device.platformName : 'BMO Device', true);

      // Otomatis minta status Wi-Fi terbaru dari BMO saat baru terhubung
      Future.delayed(const Duration(milliseconds: 350), () {
        if (isConnected) {
          requestWifiStatus();
        }
      });

      // Pantau jika terputus
      _connectionSub?.cancel();
      _connectionSub = device.connectionState.listen((state) {
        if (state == BluetoothConnectionState.disconnected) {
          _connectedDevice = null;
          _rxCharacteristic = null;
          _txCharacteristic = null;
          _wifiState = 'UNKNOWN';
          _wifiIp = null;
          _addLog('Koneksi Terputus', 'BMO offline atau di luar jangkauan', false);
          notifyListeners();
        }
      });

      return true;
    } catch (e) {
      debugPrint('[BLE] Connection error: $e');
      _addLog('Gagal Terhubung', e.toString(), false);
      // Jangan putuskan koneksi sistem jika kegagalan hanya internal discovery
      try {
        await device.disconnect();
      } catch (_) {}
      _connectedDevice = null;
      _rxCharacteristic = null;
      _txCharacteristic = null;
      notifyListeners();
      return false;
    }
  }

  Future<void> disconnect() async {
    if (_connectedDevice != null) {
      final dev = _connectedDevice!;
      _connectedDevice = null;
      _rxCharacteristic = null;
      _txCharacteristic = null;
      notifyListeners();
      try {
        await dev.disconnect();
      } catch (_) {}
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

    final trimmed = msg.trim();
    if (trimmed.startsWith('[WIFI]')) {
      final payload = trimmed.substring(6).trim();
      if (payload.startsWith('STATUS:')) {
        final statusPart = payload.substring(7).trim();
        final parts = statusPart.split('|');
        final state = parts[0].toUpperCase();
        _wifiState = state;
        if (state == 'CONNECTED' && parts.length > 1) {
          _wifiIp = parts[1].trim();
        } else if (state == 'CONNECTING' && parts.length > 1) {
          _wifiSsid = parts[1].trim();
        } else if (state == 'FAILED' && parts.length > 1) {
          _wifiSsid = parts[1].trim();
        } else if (state == 'OFF' || state == 'DISCONNECTED') {
          _wifiIp = null;
        }
      } else if (payload.startsWith('CONNECTED:')) {
        _wifiState = 'CONNECTED';
        _wifiIp = payload.substring(10).trim();
      } else if (payload == 'CONNECTING' || payload == 'SAVED_CONNECTING') {
        _wifiState = 'CONNECTING';
      } else if (payload == 'FAILED') {
        _wifiState = 'FAILED';
      } else if (payload == 'DISCONNECTED') {
        _wifiState = 'DISCONNECTED';
        _wifiIp = null;
      }
      notifyListeners();
    }
  }

  Future<bool> requestWifiStatus() {
    return sendCommand(
      '[WIFI] STATUS',
      logTitle: 'Cek Wi-Fi',
      logDetail: 'Permintaan status koneksi',
    );
  }

  Future<bool> disconnectWifi() {
    return sendCommand(
      '[WIFI] DISCONNECT',
      logTitle: 'Putuskan Wi-Fi',
      logDetail: 'Mematikan koneksi Wi-Fi BMO',
    );
  }

  Future<bool> setWeatherCity(String city) {
    return sendCommand(
      '[WEATHER] CITY:$city',
      logTitle: 'Atur Kota Cuaca',
      logDetail: 'Lokasi: $city',
    );
  }

  Future<bool> sendSpotifyTrack(String title, String artist) {
    return sendCommand(
      '[SPOTIFY] $title|$artist',
      logTitle: 'Spotify BMO',
      logDetail: '$title - $artist',
    );
  }

  Future<bool> sendSpotifyLyric(String currentLine, [String nextLine = '']) {
    final payload = nextLine.isNotEmpty ? '$currentLine|$nextLine' : currentLine;
    return sendCommand(
      '[LYRIC] $payload',
      logTitle: 'Lirik Lagu BMO',
      logDetail: currentLine,
    );
  }

  Future<bool> sendLyricFetchRequest(String title, String artist) {
    return sendCommand(
      '[LYRIC_FETCH] $title|$artist',
      logTitle: 'Fetch Lirik via WiFi',
      logDetail: '$title - $artist',
    );
  }

  Future<bool> sendCustomName(String name) {
    return sendCommand(
      '[NAME] $name',
      logTitle: 'Ubah Nama BMO',
      logDetail: name,
    );
  }

  Future<bool> sendCanvasClear([bool whiteBackground = true]) {
    return sendCommand(
      '[DRAW] CLEAR:${whiteBackground ? 1 : 0}',
      logTitle: 'Kanvas BMO',
      logDetail: 'Bersihkan Layar',
    );
  }

  Future<bool> sendCanvasExit() {
    return sendCommand(
      '[DRAW] EXIT',
      logTitle: 'Kanvas BMO',
      logDetail: 'Keluar Kanvas',
    );
  }

  Future<bool> sendCanvasInvert(bool invert) {
    return sendCommand(
      '[DRAW] INVERT:${invert ? 1 : 0}',
      logTitle: 'Kanvas BMO',
      logDetail: invert ? 'Invert Hitam' : 'Normal Putih',
    );
  }

  Future<bool> sendCanvasLine(int x0, int y0, int x1, int y1) {
    return sendCommand(
      '[DRAW] L:$x0,$y0,$x1,$y1',
      logTitle: 'Garis Kanvas',
      logDetail: '($x0,$y0)->($x1,$y1)',
    );
  }

  Future<void> sendFullCanvasBitmap(List<int> bytes1024) async {
    if (bytes1024.length != 1024) return;
    for (int page = 0; page < 8; page++) {
      final start = page * 128;
      final end = start + 128;
      final chunk = bytes1024.sublist(start, end);
      final hex = chunk.map((b) => b.toRadixString(16).padLeft(2, '0')).join();
      await sendCommand(
        '[DRAW] P:$page:$hex',
        logTitle: 'Kirim Gambar',
        logDetail: 'Halaman ${page + 1}/8',
      );
      await Future.delayed(const Duration(milliseconds: 15));
    }
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
