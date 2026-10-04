import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../models/bmo_mood.dart';
import '../services/ble_service.dart';
import '../theme/bmo_theme.dart';
import '../widgets/bmo_face_avatar.dart';
import '../widgets/glass_container.dart';

class SimulatorTab extends StatefulWidget {
  const SimulatorTab({super.key});

  @override
  State<SimulatorTab> createState() => _SimulatorTabState();
}

class _SimulatorTabState extends State<SimulatorTab> {
  final BleService _ble = BleService();
  String _activeMood = 'happy';
  String _lastActionStatus = 'Sentuh tombol di bawah untuk mengontrol BMO.';

  void _triggerMood(BmoMoodItem item) {
    HapticFeedback.mediumImpact();
    setState(() {
      _activeMood = item.id;
      _lastActionStatus = 'Ekspresi ${item.label} dikirim ke BMO.';
    });

    _ble.sendCommand(
      item.bleCommand,
      logTitle: 'Ekspresi BMO',
      logDetail: '${item.emoji} ${item.label}',
    );
  }

  void _triggerTouch(BmoMoodItem item) {
    HapticFeedback.heavyImpact();
    setState(() {
      _activeMood = item.id;
      _lastActionStatus = 'Simulasi sentuhan ${item.label} dikirim.';
    });

    _ble.sendCommand(
      item.bleCommand,
      logTitle: 'Sentuhan Sensor BMO',
      logDetail: '${item.emoji} ${item.label}',
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
              'BMO Simulator',
              style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                    fontWeight: FontWeight.bold,
                    color: BmoTheme.textPrimary,
                  ),
            ),
            const SizedBox(height: 4),
            const Text(
              'Kendali ekspresi wajah & simulasi sensor sentuh secara langsung.',
              style: TextStyle(fontSize: 14, color: BmoTheme.textSecondary),
            ),
            const SizedBox(height: 20),

            // Live Virtual BMO Screen
            Center(
              child: BmoFaceAvatar(
                mood: _activeMood,
                width: 220,
                height: 160,
              ),
            ),
            const SizedBox(height: 12),
            Center(
              child: Text(
                _lastActionStatus,
                style: const TextStyle(
                  fontSize: 13,
                  color: BmoTheme.textMuted,
                  fontStyle: FontStyle.italic,
                ),
                textAlign: TextAlign.center,
              ),
            ),
            const SizedBox(height: 24),

            // Section 1: Moods Grid
            const Text(
              'Pilihan Ekspresi Wajah (12 Moods)',
              style: TextStyle(
                fontSize: 16,
                fontWeight: FontWeight.w600,
                color: BmoTheme.textPrimary,
              ),
            ),
            const SizedBox(height: 12),
            GridView.builder(
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                crossAxisCount: 3,
                crossAxisSpacing: 10,
                mainAxisSpacing: 10,
                childAspectRatio: 1.25,
              ),
              itemCount: BmoMoods.all.length,
              itemBuilder: (context, index) {
                final item = BmoMoods.all[index];
                final isSelected = _activeMood == item.id;

                return GlassContainer(
                  padding: const EdgeInsets.all(8),
                  borderRadius: 14,
                  fill: isSelected ? BmoTheme.primary.withOpacity(0.35) : BmoTheme.glassFill,
                  borderColor: isSelected ? BmoTheme.primaryLight : BmoTheme.glassBorder,
                  onTap: () => _triggerMood(item),
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Text(item.emoji, style: const TextStyle(fontSize: 22)),
                      const SizedBox(height: 4),
                      Text(
                        item.label,
                        style: TextStyle(
                          fontSize: 12,
                          fontWeight: isSelected ? FontWeight.bold : FontWeight.w500,
                          color: isSelected ? Colors.white : BmoTheme.textPrimary,
                        ),
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                      ),
                    ],
                  ),
                );
              },
            ),
            const SizedBox(height: 24),

            // Section 2: Touch Sensors Grid
            const Text(
              'Simulasi Sensor Sentuh Fisik (TTP223)',
              style: TextStyle(
                fontSize: 16,
                fontWeight: FontWeight.w600,
                color: BmoTheme.textPrimary,
              ),
            ),
            const SizedBox(height: 12),
            GridView.builder(
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                crossAxisCount: 2,
                crossAxisSpacing: 12,
                mainAxisSpacing: 12,
                childAspectRatio: 2.2,
              ),
              itemCount: BmoMoods.touches.length,
              itemBuilder: (context, index) {
                final item = BmoMoods.touches[index];

                return GlassContainer(
                  borderRadius: 14,
                  fill: const Color(0x2A2A6AEB),
                  borderColor: BmoTheme.primary.withOpacity(0.5),
                  onTap: () => _triggerTouch(item),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Text(item.emoji, style: const TextStyle(fontSize: 20)),
                      const SizedBox(width: 8),
                      Flexible(
                        child: Text(
                          item.label,
                          style: const TextStyle(
                            fontSize: 13,
                            fontWeight: FontWeight.w600,
                            color: Colors.white,
                          ),
                          overflow: TextOverflow.ellipsis,
                        ),
                      ),
                    ],
                  ),
                );
              },
            ),

            if (!isConnected) ...[
              const SizedBox(height: 20),
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: BmoTheme.statusConnecting.withOpacity(0.15),
                  borderRadius: BorderRadius.circular(12),
                  border: Border.all(color: BmoTheme.statusConnecting.withOpacity(0.3)),
                ),
                child: const Row(
                  children: [
                    Icon(Icons.info_outline, color: BmoTheme.statusConnecting, size: 20),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(
                        'BMO belum terhubung via Bluetooth. Hubungkan BMO di tab Pengaturan untuk melihat respon langsung.',
                        style: TextStyle(fontSize: 12, color: BmoTheme.statusConnecting),
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ],
        );
      },
    );
  }
}
