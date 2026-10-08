import 'dart:ui';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../theme/bmo_theme.dart';
import 'home_tab.dart';
import 'simulator_tab.dart';
import 'wifi_provisioning_tab.dart';
import 'settings_tab.dart';

class MainScaffold extends StatefulWidget {
  const MainScaffold({super.key});

  @override
  State<MainScaffold> createState() => _MainScaffoldState();
}

class _MainScaffoldState extends State<MainScaffold> {
  int _currentIndex = 0;

  @override
  Widget build(BuildContext context) {
    final List<Widget> pages = [
      HomeTab(
        onNavigateToSettings: () => setState(() => _currentIndex = 3),
        onNavigateToSimulator: () => setState(() => _currentIndex = 1),
        onNavigateToWifi: () => setState(() => _currentIndex = 2),
      ),
      const SimulatorTab(),
      const WifiProvisioningTab(),
      const SettingsTab(),
    ];

    return Scaffold(
      body: Stack(
        children: [
          // Background Glow Orbs for Glassmorphism
          Positioned(
            top: -60,
            left: -40,
            child: Container(
              width: 260,
              height: 260,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: BmoTheme.primary.withOpacity(0.18),
              ),
            ),
          ),
          Positioned(
            top: 240,
            right: -60,
            child: Container(
              width: 220,
              height: 220,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: BmoTheme.bmoMint.withOpacity(0.12),
              ),
            ),
          ),

          // Main Tab Content
          SafeArea(
            bottom: false,
            child: IndexedStack(
              index: _currentIndex,
              children: pages,
            ),
          ),

          // Floating Frosted Glass Bottom Navigation Bar
          Positioned(
            left: 20,
            right: 20,
            bottom: 24,
            child: ClipRRect(
              borderRadius: BorderRadius.circular(24),
              child: BackdropFilter(
                filter: ImageFilter.blur(sigmaX: 18, sigmaY: 18),
                child: Container(
                  height: 68,
                  decoration: BoxDecoration(
                    color: const Color(0x35142238),
                    borderRadius: BorderRadius.circular(24),
                    border: Border.all(color: BmoTheme.glassBorder, width: 1),
                  ),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.spaceAround,
                    children: [
                      _navItem(0, Icons.home_outlined, Icons.home_rounded, 'Beranda'),
                      _navItem(1, Icons.mood_outlined, Icons.mood_rounded, 'Simulator'),
                      _navItem(2, Icons.wifi_rounded, Icons.wifi_rounded, 'Wi-Fi'),
                      _navItem(3, Icons.settings_outlined, Icons.settings_rounded, 'Setelan'),
                    ],
                  ),
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _navItem(int index, IconData icon, IconData activeIcon, String label) {
    final isSelected = _currentIndex == index;

    return GestureDetector(
      onTap: () {
        HapticFeedback.selectionClick();
        setState(() => _currentIndex = index);
      },
      behavior: HitTestBehavior.opaque,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 200),
        padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
        decoration: BoxDecoration(
          color: isSelected ? BmoTheme.primary.withOpacity(0.25) : Colors.transparent,
          borderRadius: BorderRadius.circular(16),
          border: isSelected ? Border.all(color: BmoTheme.primaryLight.withOpacity(0.4)) : null,
        ),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(
              isSelected ? activeIcon : icon,
              size: 22,
              color: isSelected ? Colors.white : BmoTheme.textSecondary,
            ),
            const SizedBox(height: 3),
            Text(
              label,
              style: TextStyle(
                fontSize: 11,
                fontWeight: isSelected ? FontWeight.bold : FontWeight.w500,
                color: isSelected ? Colors.white : BmoTheme.textSecondary,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
