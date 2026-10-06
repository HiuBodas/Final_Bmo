import 'dart:async';
import 'package:flutter/services.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'ble_service.dart';

class RealtimeNotificationService {
  static final RealtimeNotificationService _instance = RealtimeNotificationService._internal();
  factory RealtimeNotificationService() => _instance;
  RealtimeNotificationService._internal();

  static const MethodChannel _controlChannel = MethodChannel('com.bmo.companion/control');
  static const EventChannel _eventChannel = EventChannel('com.bmo.companion/notifications');

  StreamSubscription? _eventSubscription;
  final BleService _ble = BleService();

  bool _isAutoForwardEnabled = true;
  bool _filterPopularOnly = false;
  String _lastSentSignature = '';
  DateTime _lastSentTime = DateTime.fromMillisecondsSinceEpoch(0);

  bool get isAutoForwardEnabled => _isAutoForwardEnabled;
  bool get filterPopularOnly => _filterPopularOnly;

  // Callback untuk UI jika ada notifikasi baru masuk
  Function(Map<dynamic, dynamic>)? onNotificationReceived;

  Future<void> init() async {
    final prefs = await SharedPreferences.getInstance();
    _isAutoForwardEnabled = prefs.getBool('auto_forward_notif') ?? true;
    _filterPopularOnly = prefs.getBool('filter_popular_notif') ?? false;

    // Mulai dengarkan event dari Android Native Listener
    _eventSubscription?.cancel();
    _eventSubscription = _eventChannel.receiveBroadcastStream().listen(
      (dynamic event) {
        if (event is Map) {
          _handleNativeEvent(event);
        }
      },
      onError: (dynamic error) {
        // Ignored or logged
      },
    );
  }

  Future<bool> isListenerPermissionGranted() async {
    try {
      final bool granted = await _controlChannel.invokeMethod('isNotificationListenerEnabled');
      return granted;
    } catch (_) {
      return false;
    }
  }

  Future<void> openListenerSettings() async {
    try {
      await _controlChannel.invokeMethod('openNotificationListenerSettings');
    } catch (_) {}
  }

  Future<void> openAppDetailsSettings() async {
    try {
      await _controlChannel.invokeMethod('openAppDetailsSettings');
    } catch (_) {}
  }

  Future<void> setAutoForward(bool enabled) async {
    _isAutoForwardEnabled = enabled;
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool('auto_forward_notif', enabled);
  }

  Future<void> setFilterPopularOnly(bool enabled) async {
    _filterPopularOnly = enabled;
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool('filter_popular_notif', enabled);
  }

  Future<void> startKeepAliveService() async {
    try {
      await _controlChannel.invokeMethod('startForegroundService');
    } catch (_) {}
  }

  Future<void> stopKeepAliveService() async {
    try {
      await _controlChannel.invokeMethod('stopForegroundService');
    } catch (_) {}
  }

  void _handleNativeEvent(Map<dynamic, dynamic> data) {
    final type = data['type'] as String?;
    if (type == 'notification') {
      final pkg = (data['package'] as String? ?? '').toLowerCase();
      final appName = data['app'] as String? ?? 'NOTIF';
      final title = data['title'] as String? ?? '';
      final text = data['text'] as String? ?? '';

      // Panggil callback UI jika ada
      onNotificationReceived?.call(data);

      if (!_isAutoForwardEnabled) return;
      if (!_ble.isConnected) return;

      // Filter deduplikasi cepat (pesan yang sama persis dalam 2.5 detik)
      final signature = '$pkg|$title|$text';
      final now = DateTime.now();
      if (_lastSentSignature == signature && now.difference(_lastSentTime).inMilliseconds < 2500) {
        return;
      }
      _lastSentSignature = signature;
      _lastSentTime = now;

      // Filter aplikasi jika mode populer aktif
      if (_filterPopularOnly) {
        final isPopular = pkg.contains('whatsapp') ||
            pkg.contains('telegram') ||
            pkg.contains('instagram') ||
            pkg.contains('mms') ||
            pkg.contains('messaging') ||
            pkg.contains('discord') ||
            pkg.contains('android.dialer') ||
            pkg.contains('telecom');
        if (!isPopular) return;
      }

      // Deteksi Tag BMO agar ikon vector ESP32 menyala sesuai aplikasi
      String appTag = 'PESAN';
      if (pkg.contains('whatsapp')) {
        appTag = 'WA';
      } else if (pkg.contains('telegram')) {
        appTag = 'TELE';
      } else if (pkg.contains('instagram')) {
        appTag = 'IG';
      } else if (pkg.contains('dialer') || pkg.contains('telecom') || pkg.contains('incall')) {
        appTag = 'CALL';
      } else if (pkg.contains('sms') || pkg.contains('mms') || pkg.contains('messaging')) {
        appTag = 'SMS';
      } else if (pkg.contains('gmail') || pkg.contains('mail')) {
        appTag = 'GMAIL';
      } else {
        // Ambil singkatan nama app jika memungkinkan
        if (appName.isNotEmpty && appName.length <= 6) {
          appTag = appName.toUpperCase();
        }
      }

      // Bersihkan teks sender dan isi pesan
      String sender = title.trim();
      if (sender.isEmpty) sender = appName;

      String messageBody = text.trim();
      if (messageBody.isEmpty) messageBody = 'Notifikasi Baru';

      // Potong jika terlalu panjang agar BLE MTU aman dan ramah OLED
      if (sender.length > 30) sender = sender.substring(0, 30);
      if (messageBody.length > 90) messageBody = messageBody.substring(0, 90);

      // Format BLE yang didukung BleManager.cpp: [TAG] Pengirim: Pesan
      final payload = '[$appTag] $sender: $messageBody';

      _ble.sendCommand(
        payload,
        logTitle: 'Notif Otomatis ($appTag)',
        logDetail: '$sender: $messageBody',
      );
    }
  }
}
