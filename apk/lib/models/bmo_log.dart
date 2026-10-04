class BmoLogEntry {
  final String title;
  final String message;
  final DateTime timestamp;
  final bool isSuccess;

  BmoLogEntry({
    required this.title,
    required this.message,
    DateTime? timestamp,
    this.isSuccess = true,
  }) : timestamp = timestamp ?? DateTime.now();

  String get timeFormatted {
    final h = timestamp.hour.toString().padLeft(2, '0');
    final m = timestamp.minute.toString().padLeft(2, '0');
    final s = timestamp.second.toString().padLeft(2, '0');
    return '$h:$m:$s';
  }
}
