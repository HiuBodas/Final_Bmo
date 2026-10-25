import 'dart:convert';
import 'dart:io';

class LyricLine {
  final int timestampMs;
  final String text;

  LyricLine({required this.timestampMs, required this.text});

  String get timeFormatted {
    final totalSeconds = timestampMs ~/ 1000;
    final m = totalSeconds ~/ 60;
    final s = totalSeconds % 60;
    return '${m.toString().padLeft(2, '0')}:${s.toString().padLeft(2, '0')}';
  }
}

class LyricResult {
  final String trackName;
  final String artistName;
  final List<LyricLine> lines;
  final bool isSynced;

  LyricResult({
    required this.trackName,
    required this.artistName,
    required this.lines,
    required this.isSynced,
  });
}

class LyricsService {
  static final HttpClient _httpClient = HttpClient()
    ..connectionTimeout = const Duration(seconds: 10);

  /// Mengambil lirik lagu gratis dari LRCLIB (tanpa API key).
  static Future<LyricResult?> fetchLyrics(String title, String artist) async {
    try {
      final queryParams = {
        'track_name': title.trim(),
        if (artist.trim().isNotEmpty) 'artist_name': artist.trim(),
      };

      final uri = Uri.https('lrclib.net', '/api/get', queryParams);
      final request = await _httpClient.getUrl(uri);
      request.headers.set('User-Agent', 'BMO-Companion-App/1.0 (Android)');

      final response = await request.close();
      if (response.statusCode != 200) {
        // Coba endpoint search jika endpoint get belum menemukan
        return await _searchLyricsFallback(title, artist);
      }

      final responseBody = await response.transform(utf8.decoder).join();
      final Map<String, dynamic> data = jsonDecode(responseBody);

      return _parseLrcData(data);
    } catch (e) {
      // Fallback search
      return await _searchLyricsFallback(title, artist);
    }
  }

  static Future<LyricResult?> _searchLyricsFallback(String title, String artist) async {
    try {
      final q = '$title $artist'.trim();
      final uri = Uri.https('lrclib.net', '/api/search', {'q': q});
      final request = await _httpClient.getUrl(uri);
      request.headers.set('User-Agent', 'BMO-Companion-App/1.0 (Android)');

      final response = await request.close();
      if (response.statusCode != 200) return null;

      final responseBody = await response.transform(utf8.decoder).join();
      final List<dynamic> results = jsonDecode(responseBody);
      if (results.isEmpty) return null;

      return _parseLrcData(results.first as Map<String, dynamic>);
    } catch (e) {
      return null;
    }
  }

  static LyricResult? _parseLrcData(Map<String, dynamic> data) {
    final trackName = data['trackName'] ?? data['name'] ?? 'Lagu';
    final artistName = data['artistName'] ?? '';
    final syncedLyrics = data['syncedLyrics'] as String?;
    final plainLyrics = data['plainLyrics'] as String?;

    if (syncedLyrics != null && syncedLyrics.trim().isNotEmpty) {
      final parsed = _parseLrcFormat(syncedLyrics);
      if (parsed.isNotEmpty) {
        return LyricResult(
          trackName: trackName,
          artistName: artistName,
          lines: parsed,
          isSynced: true,
        );
      }
    }

    if (plainLyrics != null && plainLyrics.trim().isNotEmpty) {
      final rawLines = plainLyrics.split('\n');
      final lines = <LyricLine>[];
      for (final raw in rawLines) {
        final trimmed = raw.trim();
        if (trimmed.isNotEmpty) {
          lines.add(LyricLine(timestampMs: 0, text: trimmed));
        }
      }
      if (lines.isNotEmpty) {
        return LyricResult(
          trackName: trackName,
          artistName: artistName,
          lines: lines,
          isSynced: false,
        );
      }
    }

    return null;
  }

  static List<LyricLine> _parseLrcFormat(String lrc) {
    final lines = <LyricLine>[];
    final regex = RegExp(r'\[(\d{2}):(\d{2})\.(\d{2,3})\](.*)');

    for (final rawLine in lrc.split('\n')) {
      final match = regex.firstMatch(rawLine.trim());
      if (match != null) {
        final min = int.tryParse(match.group(1) ?? '0') ?? 0;
        final sec = int.tryParse(match.group(2) ?? '0') ?? 0;
        final frac = match.group(3) ?? '0';
        final ms = frac.length == 2
            ? (int.tryParse(frac) ?? 0) * 10
            : (int.tryParse(frac) ?? 0);

        final totalMs = (min * 60 * 1000) + (sec * 1000) + ms;
        final text = match.group(4)?.trim() ?? '';
        if (text.isNotEmpty) {
          lines.add(LyricLine(timestampMs: totalMs, text: text));
        }
      }
    }

    lines.sort((a, b) => a.timestampMs.compareTo(b.timestampMs));
    return lines;
  }
}
