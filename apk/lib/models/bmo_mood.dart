import 'package:flutter/material.dart';

class BmoMoodItem {
  final String id;
  final String label;
  final String emoji;
  final String bleCommand;
  final IconData? icon;
  final String? subtitle;

  const BmoMoodItem({
    required this.id,
    required this.label,
    this.emoji = '',
    required this.bleCommand,
    this.icon,
    this.subtitle,
  });
}

class BmoMoods {
  static const List<BmoMoodItem> all = [
    BmoMoodItem(id: 'happy', label: 'Senang', icon: Icons.sentiment_very_satisfied_rounded, bleCommand: '[BMO] MOOD:HAPPY'),
    BmoMoodItem(id: 'cool', label: 'Keren', icon: Icons.auto_awesome_rounded, bleCommand: '[BMO] MOOD:COOL'),
    BmoMoodItem(id: 'laugh', label: 'Tertawa', icon: Icons.sentiment_satisfied_alt_rounded, bleCommand: '[BMO] MOOD:LAUGH'),
    BmoMoodItem(id: 'love', label: 'Cinta', icon: Icons.favorite_rounded, bleCommand: '[BMO] MOOD:LOVE'),
    BmoMoodItem(id: 'wink', label: 'Kedip', icon: Icons.remove_red_eye_rounded, bleCommand: '[BMO] MOOD:WINK'),
    BmoMoodItem(id: 'idle', label: 'Santai', icon: Icons.sentiment_neutral_rounded, bleCommand: '[BMO] MOOD:IDLE'),
    BmoMoodItem(id: 'blink', label: 'Kedip-kedip', icon: Icons.visibility_off_rounded, bleCommand: '[BMO] MOOD:BLINK'),
    BmoMoodItem(id: 'talk', label: 'Bicara', icon: Icons.chat_bubble_outline_rounded, bleCommand: '[BMO] MOOD:TALK'),
    BmoMoodItem(id: 'listen', label: 'Mendengar', icon: Icons.headphones_rounded, bleCommand: '[BMO] MOOD:LISTEN'),
    BmoMoodItem(id: 'focused', label: 'Fokus', icon: Icons.center_focus_strong_rounded, bleCommand: '[BMO] MOOD:FOCUSED'),
    BmoMoodItem(id: 'thinking', label: 'Mikir', icon: Icons.psychology_rounded, bleCommand: '[BMO] MOOD:THINKING'),
    BmoMoodItem(id: 'surprise', label: 'Kaget', icon: Icons.priority_high_rounded, bleCommand: '[BMO] MOOD:SURPRISE'),
    BmoMoodItem(id: 'excited', label: 'Semangat', icon: Icons.celebration_rounded, bleCommand: '[BMO] MOOD:EXCITED'),
    BmoMoodItem(id: 'hungry', label: 'Lapar', icon: Icons.restaurant_rounded, bleCommand: '[BMO] MOOD:HUNGRY'),
    BmoMoodItem(id: 'cold', label: 'Dingin', icon: Icons.ac_unit_rounded, bleCommand: '[BMO] MOOD:COLD'),
    BmoMoodItem(id: 'hot', label: 'Kepanasan', icon: Icons.whatshot_rounded, bleCommand: '[BMO] MOOD:HOT'),
    BmoMoodItem(id: 'sick', label: 'Sakit', icon: Icons.healing_rounded, bleCommand: '[BMO] MOOD:SICK'),
    BmoMoodItem(id: 'dizzy', label: 'Pusing', icon: Icons.change_circle_rounded, bleCommand: '[BMO] MOOD:DIZZY'),
    BmoMoodItem(id: 'glitch', label: 'Glitch', icon: Icons.developer_board_rounded, bleCommand: '[BMO] MOOD:GLITCH'),
    BmoMoodItem(id: 'confused', label: 'Bingung', icon: Icons.help_outline_rounded, bleCommand: '[BMO] MOOD:CONFUSED'),
    BmoMoodItem(id: 'bored', label: 'Bosan', icon: Icons.hourglass_empty_rounded, bleCommand: '[BMO] MOOD:BORED'),
    BmoMoodItem(id: 'sad', label: 'Sedih', icon: Icons.sentiment_dissatisfied_rounded, bleCommand: '[BMO] MOOD:SAD'),
    BmoMoodItem(id: 'scared', label: 'Takut', icon: Icons.report_problem_rounded, bleCommand: '[BMO] MOOD:SCARED'),
    BmoMoodItem(id: 'angry', label: 'Marah', icon: Icons.mood_bad_rounded, bleCommand: '[BMO] MOOD:ANGRY'),
    BmoMoodItem(id: 'wake', label: 'Bangun', icon: Icons.wb_sunny_rounded, bleCommand: '[BMO] MOOD:WAKE'),
    BmoMoodItem(id: 'sleepy', label: 'Tidur', icon: Icons.bedtime_rounded, bleCommand: '[BMO] MOOD:SLEEPY'),
  ];

  static const List<BmoMoodItem> touches = [
    BmoMoodItem(
      id: 'poke',
      label: 'Ketukan Singkat',
      subtitle: 'Short Tap',
      icon: Icons.touch_app_rounded,
      bleCommand: '[BMO] TOUCH:POKE',
    ),
    BmoMoodItem(
      id: 'hold',
      label: 'Sentuhan Lembut',
      subtitle: 'Soft Hold',
      icon: Icons.pan_tool_alt_rounded,
      bleCommand: '[BMO] TOUCH:HOLD',
    ),
    BmoMoodItem(
      id: 'long_hold',
      label: 'Tekan & Tahan',
      subtitle: 'Long Press',
      icon: Icons.fingerprint_rounded,
      bleCommand: '[BMO] TOUCH:LONG_HOLD',
    ),
    BmoMoodItem(
      id: 'tickle',
      label: 'Ketukan Beruntun',
      subtitle: 'Rapid Multi-Tap',
      icon: Icons.vibration_rounded,
      bleCommand: '[BMO] TOUCH:TICKLE',
    ),
  ];
}
