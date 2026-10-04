class BmoMoodItem {
  final String id;
  final String label;
  final String emoji;
  final String bleCommand;

  const BmoMoodItem({
    required this.id,
    required this.label,
    required this.emoji,
    required this.bleCommand,
  });
}

class BmoMoods {
  static const List<BmoMoodItem> all = [
    BmoMoodItem(id: 'happy', label: 'Senang', emoji: '😃', bleCommand: '[BMO] MOOD:HAPPY'),
    BmoMoodItem(id: 'wink', label: 'Kedip', emoji: '😉', bleCommand: '[BMO] MOOD:WINK'),
    BmoMoodItem(id: 'love', label: 'Cinta', emoji: '❤️', bleCommand: '[BMO] MOOD:LOVE'),
    BmoMoodItem(id: 'sleepy', label: 'Tidur', emoji: '😴', bleCommand: '[BMO] MOOD:SLEEPY'),
    BmoMoodItem(id: 'surprise', label: 'Kaget', emoji: '😲', bleCommand: '[BMO] MOOD:SURPRISE'),
    BmoMoodItem(id: 'cool', label: 'Keren', emoji: '🕶️', bleCommand: '[BMO] MOOD:COOL'),
    BmoMoodItem(id: 'sad', label: 'Sedih', emoji: '😢', bleCommand: '[BMO] MOOD:SAD'),
    BmoMoodItem(id: 'angry', label: 'Marah', emoji: '😡', bleCommand: '[BMO] MOOD:ANGRY'),
    BmoMoodItem(id: 'hungry', label: 'Lapar', emoji: '😋', bleCommand: '[BMO] MOOD:HUNGRY'),
    BmoMoodItem(id: 'dizzy', label: 'Pusing', emoji: '😵', bleCommand: '[BMO] MOOD:DIZZY'),
    BmoMoodItem(id: 'glitch', label: 'Glitch', emoji: '🌀', bleCommand: '[BMO] MOOD:GLITCH'),
    BmoMoodItem(id: 'idle', label: 'Normal', emoji: '😐', bleCommand: '[BMO] MOOD:IDLE'),
  ];

  static const List<BmoMoodItem> touches = [
    BmoMoodItem(id: 'poke', label: 'Colek (Poke)', emoji: '👉', bleCommand: '[BMO] TOUCH:POKE'),
    BmoMoodItem(id: 'hold', label: 'Elus (Pet)', emoji: '💆', bleCommand: '[BMO] TOUCH:HOLD'),
    BmoMoodItem(id: 'long_hold', label: 'Peluk (Hug)', emoji: '💖', bleCommand: '[BMO] TOUCH:LONG_HOLD'),
    BmoMoodItem(id: 'tickle', label: 'Gelitik (Tickle)', emoji: '😂', bleCommand: '[BMO] TOUCH:TICKLE'),
  ];
}
