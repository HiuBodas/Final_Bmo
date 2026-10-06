import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'services/ble_service.dart';
import 'services/notification_service.dart';
import 'theme/bmo_theme.dart';
import 'screens/main_scaffold.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  
  // Set transparent system navigation & status bar
  SystemChrome.setSystemUIOverlayStyle(
    const SystemUiOverlayStyle(
      statusBarColor: Colors.transparent,
      statusBarIconBrightness: Brightness.light,
      systemNavigationBarColor: Colors.transparent,
      systemNavigationBarIconBrightness: Brightness.light,
    ),
  );

  await BleService().init();
  final notifService = RealtimeNotificationService();
  await notifService.init();
  await notifService.startKeepAliveService();

  runApp(const BmoCompanionApp());
}

class BmoCompanionApp extends StatelessWidget {
  const BmoCompanionApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'BMO Companion',
      debugShowCheckedModeBanner: false,
      theme: BmoTheme.themeData,
      home: const MainScaffold(),
    );
  }
}
