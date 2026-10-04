import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

class BmoTheme {
  // Palette: Pearl + Cobalt + BMO Mint Accent
  static const Color canvas = Color(0xFF0C131F);
  static const Color surface = Color(0xFF152238);
  static const Color surfaceElevated = Color(0xFF1C2C47);
  
  static const Color primary = Color(0xFF2A6AEB);
  static const Color primaryLight = Color(0xFF60A5FA);
  static const Color bmoMint = Color(0xFF5DCBC2);
  static const Color bmoMouth = Color(0xFF2C443B);

  static const Color glassFill = Color(0x14FFFFFF);
  static const Color glassFillElevated = Color(0x24FFFFFF);
  static const Color glassBorder = Color(0x2EFFFFFF);
  static const Color glassBorderSubtle = Color(0x1AFFFFFF);

  static const Color textPrimary = Color(0xFFF8FAFC);
  static const Color textSecondary = Color(0xFF94A3B8);
  static const Color textMuted = Color(0xFF64748B);

  static const Color statusOnline = Color(0xFF10B981);
  static const Color statusOffline = Color(0xFFEF4444);
  static const Color statusConnecting = Color(0xFFF59E0B);

  static ThemeData get themeData {
    return ThemeData(
      brightness: Brightness.dark,
      scaffoldBackgroundColor: canvas,
      primaryColor: primary,
      colorScheme: const ColorScheme.dark(
        primary: primary,
        secondary: bmoMint,
        surface: surface,
      ),
      textTheme: GoogleFonts.plusJakartaSansTextTheme(
        ThemeData.dark().textTheme,
      ),
      appBarTheme: const AppBarTheme(
        backgroundColor: Colors.transparent,
        elevation: 0,
        centerTitle: false,
      ),
      useMaterial3: true,
    );
  }
}
