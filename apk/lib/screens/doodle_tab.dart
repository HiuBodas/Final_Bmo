import 'dart:async';
import 'dart:math';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../services/ble_service.dart';
import '../theme/bmo_theme.dart';
import '../widgets/glass_container.dart';

class DoodleTab extends StatefulWidget {
  const DoodleTab({super.key});

  @override
  State<DoodleTab> createState() => _DoodleTabState();
}

class _DoodleTabState extends State<DoodleTab> {
  final BleService _ble = BleService();

  // Matriks Piksel 128 x 64 (true = piksel hitam digambar, false = background putih)
  final List<bool> _pixels = List<bool>.filled(128 * 64, false);

  bool _isEraser = false;
  int _brushSize = 1; // 1 = normal (1px), 2 = tebal (2x2px)
  bool _isInverted = false;
  bool _isSending = false;
  String _statusText = 'Gambar dengan jarimu pada kanvas di bawah.';

  Point? _lastPoint;

  @override
  void initState() {
    super.initState();
  }

  void _clearCanvas() {
    HapticFeedback.lightImpact();
    setState(() {
      _pixels.fillRange(0, _pixels.length, false);
      _statusText = 'Kanvas telah dibersihkan.';
    });
    if (_ble.isConnected) {
      _ble.sendCanvasClear(!_isInverted);
    }
  }

  void _toggleInvert() {
    HapticFeedback.selectionClick();
    setState(() {
      _isInverted = !_isInverted;
      _statusText = _isInverted ? 'Warna dibalik: Background Hitam' : 'Warna normal: Background Putih';
    });
    if (_ble.isConnected) {
      _ble.sendCanvasInvert(_isInverted);
    }
  }

  void _exitDoodleOnBmo() {
    HapticFeedback.mediumImpact();
    if (_ble.isConnected) {
      _ble.sendCanvasExit();
    }
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('Layar BMO kembali ke ekspresi wajah normal.')),
    );
  }

  void _applyBrush(int cx, int cy, bool val) {
    for (int dy = 0; dy < _brushSize; dy++) {
      for (int dx = 0; dx < _brushSize; dx++) {
        final x = cx + dx;
        final y = cy + dy;
        if (x >= 0 && x < 128 && y >= 0 && y < 64) {
          _pixels[x + y * 128] = val;
        }
      }
    }
  }

  void _drawLineOnMatrix(int x0, int y0, int x1, int y1, bool val) {
    int dx = (x1 - x0).abs();
    int dy = -(y1 - y0).abs();
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
      _applyBrush(x0, y0, val);
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 >= dy) {
        err += dy;
        x0 += sx;
      }
      if (e2 <= dx) {
        err += dx;
        y0 += sy;
      }
    }
  }

  void _handleTouch(Offset localPos, Size size, bool isStart) {
    final scaleX = 128.0 / size.width;
    final scaleY = 64.0 / size.height;

    final px = (localPos.dx * scaleX).clamp(0.0, 127.0).toInt();
    final py = (localPos.dy * scaleY).clamp(0.0, 63.0).toInt();

    final val = !_isEraser;

    final prevPoint = _lastPoint;
    setState(() {
      if (isStart || prevPoint == null) {
        _applyBrush(px, py, val);
      } else {
        _drawLineOnMatrix(prevPoint.x, prevPoint.y, px, py, val);
      }
      _lastPoint = Point(px, py);
    });

    // Kirim garis cepat real-time via BLE jika terhubung
    if (_ble.isConnected && prevPoint != null && !isStart) {
      _ble.sendCanvasLine(prevPoint.x, prevPoint.y, px, py);
    }
  }

  void _onPanEnd() async {
    _lastPoint = null;
    await _sendFullBitmapToBmo();
  }

  Future<void> _sendFullBitmapToBmo() async {
    if (!_ble.isConnected) {
      setState(() {
        _statusText = 'Gambar tersimpan di HP (BMO belum terhubung via BLE).';
      });
      return;
    }

    if (_isSending) return;
    _isSending = true;

    setState(() {
      _statusText = 'Mengirim gambar ke layar BMO...';
    });

    // Buat buffer 1024-byte format SSD1306 Page Addressing
    final bytes = List<int>.filled(1024, _isInverted ? 0x00 : 0xFF);

    for (int y = 0; y < 64; y++) {
      for (int x = 0; x < 128; x++) {
        final isDrawn = _pixels[x + y * 128];
        final page = y >> 3;
        final bit = y & 7;
        final idx = x + page * 128;

        if (_isInverted) {
          // Inverted: background hitam (0), gambar putih (1)
          if (isDrawn) bytes[idx] |= (1 << bit);
        } else {
          // Normal: background putih (1), gambar hitam (0)
          if (isDrawn) bytes[idx] &= ~(1 << bit);
        }
      }
    }

    await _ble.sendFullCanvasBitmap(bytes);

    if (mounted) {
      setState(() {
        _isSending = false;
        _statusText = 'Gambar berhasil tampil di layar BMO!';
      });
    }
  }

  void _drawPreset(String type) {
    HapticFeedback.mediumImpact();
    _pixels.fillRange(0, _pixels.length, false);

    if (type == 'heart') {
      // Gambar Hati Cantik di tengah (cx: 64, cy: 30)
      for (double t = 0; t < 6.28; t += 0.02) {
        final x = (64 + 18 * (16 * (sin(t) * sin(t) * sin(t)) / 16)).round();
        final y = (28 - (13 * cos(t) - 5 * cos(2 * t) - 2 * cos(3 * t) - cos(4 * t))).round();
        _applyBrush(x, y, true);
      }
    } else if (type == 'smile') {
      // Mata kiri & kanan
      _applyBrush(45, 24, true);
      _applyBrush(46, 24, true);
      _applyBrush(45, 25, true);
      _applyBrush(46, 25, true);
      _applyBrush(82, 24, true);
      _applyBrush(83, 24, true);
      _applyBrush(82, 25, true);
      _applyBrush(83, 25, true);
      // Senyum
      for (int x = 48; x <= 80; x++) {
        final y = 38 + ((x - 64) * (x - 64) / 45).round();
        _applyBrush(x, y, true);
        _applyBrush(x, y + 1, true);
      }
    } else if (type == 'star') {
      // Gambar Bintang
      _drawLineOnMatrix(64, 12, 70, 28, true);
      _drawLineOnMatrix(70, 28, 86, 28, true);
      _drawLineOnMatrix(86, 28, 73, 38, true);
      _drawLineOnMatrix(73, 38, 78, 54, true);
      _drawLineOnMatrix(78, 54, 64, 43, true);
      _drawLineOnMatrix(64, 43, 50, 54, true);
      _drawLineOnMatrix(50, 54, 55, 38, true);
      _drawLineOnMatrix(55, 38, 42, 28, true);
      _drawLineOnMatrix(42, 28, 58, 28, true);
      _drawLineOnMatrix(58, 28, 64, 12, true);
    } else if (type == 'cat') {
      // Wajah Kucing
      for (int x = 44; x <= 84; x++) {
        _applyBrush(x, 26, true);
        _applyBrush(x, 50, true);
      }
      for (int y = 26; y <= 50; y++) {
        _applyBrush(44, y, true);
        _applyBrush(84, y, true);
      }
      // Telinga
      _drawLineOnMatrix(44, 26, 38, 14, true);
      _drawLineOnMatrix(38, 14, 54, 26, true);
      _drawLineOnMatrix(74, 26, 90, 14, true);
      _drawLineOnMatrix(90, 14, 84, 26, true);
      // Mata & Kumis
      _applyBrush(54, 34, true);
      _applyBrush(74, 34, true);
      _applyBrush(64, 40, true);
      _drawLineOnMatrix(32, 38, 42, 40, true);
      _drawLineOnMatrix(32, 44, 42, 42, true);
      _drawLineOnMatrix(86, 40, 96, 38, true);
      _drawLineOnMatrix(86, 42, 96, 44, true);
    } else if (type == 'bmo') {
      // Badan BMO
      _drawLineOnMatrix(49, 8, 79, 8, true);
      _drawLineOnMatrix(46, 11, 46, 52, true);
      _drawLineOnMatrix(82, 11, 82, 52, true);
      _drawLineOnMatrix(49, 54, 79, 54, true);
      // Sudut Lengkung Badan
      _drawLineOnMatrix(46, 11, 49, 8, true);
      _drawLineOnMatrix(82, 11, 79, 8, true);
      _drawLineOnMatrix(46, 52, 49, 54, true);
      _drawLineOnMatrix(82, 52, 79, 54, true);
      // Layar BMO
      _drawLineOnMatrix(52, 12, 76, 12, true);
      _drawLineOnMatrix(52, 28, 76, 28, true);
      _drawLineOnMatrix(50, 14, 50, 26, true);
      _drawLineOnMatrix(78, 14, 78, 26, true);
      // Wajah Senyum di Layar
      _applyBrush(57, 18, true);
      _applyBrush(71, 18, true);
      _drawLineOnMatrix(62, 23, 66, 23, true);
      // Slot Cartridge
      _drawLineOnMatrix(52, 33, 76, 33, true);
      _drawLineOnMatrix(52, 34, 76, 34, true);
      // D-Pad Silang
      _drawLineOnMatrix(54, 40, 58, 40, true);
      _drawLineOnMatrix(54, 46, 58, 46, true);
      _drawLineOnMatrix(56, 38, 56, 48, true);
      _drawLineOnMatrix(53, 43, 59, 43, true);
      // Tombol Aksi Kanan
      _applyBrush(73, 40, true);
      _applyBrush(76, 44, true);
      _applyBrush(68, 45, true);
      // Kaki & Tangan
      _drawLineOnMatrix(54, 54, 54, 61, true);
      _drawLineOnMatrix(52, 61, 56, 61, true);
      _drawLineOnMatrix(74, 54, 74, 61, true);
      _drawLineOnMatrix(72, 61, 76, 61, true);
      _drawLineOnMatrix(46, 34, 38, 40, true);
      _drawLineOnMatrix(82, 34, 90, 40, true);
    } else if (type == 'gamepad') {
      // Frame Kontroler
      _drawLineOnMatrix(38, 18, 90, 18, true);
      _drawLineOnMatrix(38, 46, 90, 46, true);
      _drawLineOnMatrix(30, 26, 30, 38, true);
      _drawLineOnMatrix(98, 26, 98, 38, true);
      _drawLineOnMatrix(30, 26, 38, 18, true);
      _drawLineOnMatrix(98, 26, 90, 18, true);
      _drawLineOnMatrix(30, 38, 38, 46, true);
      _drawLineOnMatrix(98, 38, 90, 46, true);
      // D-Pad Silang Kiri
      _drawLineOnMatrix(43, 32, 49, 32, true);
      _drawLineOnMatrix(44, 31, 48, 31, true);
      _drawLineOnMatrix(44, 33, 48, 33, true);
      _drawLineOnMatrix(46, 29, 46, 35, true);
      _drawLineOnMatrix(45, 30, 45, 34, true);
      _drawLineOnMatrix(47, 30, 47, 34, true);
      // Tombol Aksi Kanan (4 Bulatan)
      _applyBrush(81, 26, true);
      _applyBrush(82, 26, true);
      _applyBrush(87, 32, true);
      _applyBrush(88, 32, true);
      _applyBrush(81, 38, true);
      _applyBrush(82, 38, true);
      _applyBrush(75, 32, true);
      _applyBrush(76, 32, true);
      // Tombol Select & Start
      _drawLineOnMatrix(60, 32, 63, 32, true);
      _drawLineOnMatrix(66, 32, 69, 32, true);
    } else if (type == 'lightning') {
      // Sambaran Petir Tajam
      _drawLineOnMatrix(68, 6, 52, 30, true);
      _drawLineOnMatrix(69, 6, 53, 30, true);
      _drawLineOnMatrix(52, 30, 72, 30, true);
      _drawLineOnMatrix(52, 31, 72, 31, true);
      _drawLineOnMatrix(72, 30, 56, 58, true);
      _drawLineOnMatrix(73, 30, 57, 58, true);
    } else if (type == 'music') {
      // Dua Not Balok Bertaut
      for (int x = 46; x <= 53; x++) {
        for (int y = 44; y <= 50; y++) {
          _applyBrush(x, y, true);
        }
      }
      _drawLineOnMatrix(53, 46, 53, 16, true);
      _drawLineOnMatrix(54, 46, 54, 16, true);

      for (int x = 72; x <= 79; x++) {
        for (int y = 38; y <= 44; y++) {
          _applyBrush(x, y, true);
        }
      }
      _drawLineOnMatrix(79, 40, 79, 10, true);
      _drawLineOnMatrix(80, 40, 80, 10, true);

      _drawLineOnMatrix(53, 16, 80, 10, true);
      _drawLineOnMatrix(53, 17, 80, 11, true);
      _drawLineOnMatrix(53, 18, 80, 12, true);
      _drawLineOnMatrix(53, 19, 80, 13, true);
    } else if (type == 'moon') {
      // Bulan Sabit & Bintang Mini
      for (double t = -1.6; t <= 1.6; t += 0.03) {
        final xOut = (58 + 22 * cos(t)).round();
        final yOut = (32 + 22 * sin(t)).round();
        _applyBrush(xOut, yOut, true);
        final xIn = (68 + 18 * cos(t * 0.9)).round();
        final yIn = (32 + 20 * sin(t * 0.9)).round();
        _applyBrush(xIn, yIn, true);
      }
      _applyBrush(88, 20, true);
      _drawLineOnMatrix(86, 20, 90, 20, true);
      _drawLineOnMatrix(88, 18, 88, 22, true);
    } else if (type == 'coffee') {
      // Cangkir Kopi
      _drawLineOnMatrix(48, 26, 78, 26, true);
      _drawLineOnMatrix(48, 26, 52, 50, true);
      _drawLineOnMatrix(78, 26, 74, 50, true);
      _drawLineOnMatrix(52, 50, 74, 50, true);
      // Gagang
      _drawLineOnMatrix(78, 30, 88, 32, true);
      _drawLineOnMatrix(88, 32, 88, 42, true);
      _drawLineOnMatrix(88, 42, 76, 44, true);
      // Piring Tatakan
      _drawLineOnMatrix(42, 54, 84, 54, true);
      _drawLineOnMatrix(44, 55, 82, 55, true);
      // Asap Hangat
      for (int dy = 0; dy < 8; dy++) {
        _applyBrush(58 + (sin(dy * 0.8) * 2).round(), 22 - dy, true);
        _applyBrush(68 + (sin(dy * 0.8 + 1.5) * 2).round(), 22 - dy, true);
      }
    } else if (type == 'ghost') {
      // Hantu Lucu
      for (double t = 3.14; t <= 6.28; t += 0.05) {
        final x = (64 + 18 * cos(t)).round();
        final y = (26 + 18 * sin(t)).round();
        _applyBrush(x, y, true);
      }
      _drawLineOnMatrix(46, 26, 46, 48, true);
      _drawLineOnMatrix(82, 26, 82, 48, true);
      _drawLineOnMatrix(46, 48, 52, 44, true);
      _drawLineOnMatrix(52, 44, 58, 48, true);
      _drawLineOnMatrix(58, 48, 64, 44, true);
      _drawLineOnMatrix(64, 44, 70, 48, true);
      _drawLineOnMatrix(70, 48, 76, 44, true);
      _drawLineOnMatrix(76, 44, 82, 48, true);
      for (int dy = 0; dy < 3; dy++) {
        for (int dx = 0; dx < 3; dx++) {
          _applyBrush(54 + dx, 24 + dy, true);
          _applyBrush(70 + dx, 24 + dy, true);
        }
      }
    } else if (type == 'fire') {
      // Nyala Api
      _drawLineOnMatrix(64, 10, 78, 34, true);
      _drawLineOnMatrix(78, 34, 78, 46, true);
      _drawLineOnMatrix(78, 46, 64, 54, true);
      _drawLineOnMatrix(64, 54, 50, 46, true);
      _drawLineOnMatrix(50, 46, 50, 34, true);
      _drawLineOnMatrix(50, 34, 64, 10, true);
      _drawLineOnMatrix(64, 28, 70, 40, true);
      _drawLineOnMatrix(70, 40, 64, 48, true);
      _drawLineOnMatrix(64, 48, 58, 40, true);
      _drawLineOnMatrix(58, 40, 64, 28, true);
    }

    setState(() {});
    _sendFullBitmapToBmo();
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
            // Header
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Kanvas Gambar BMO',
                      style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                            fontWeight: FontWeight.bold,
                            color: BmoTheme.textPrimary,
                          ),
                    ),
                    const SizedBox(height: 4),
                    const Text(
                      'Doodle langsung dengan jarimu ke layar OLED BMO.',
                      style: TextStyle(fontSize: 13, color: BmoTheme.textSecondary),
                    ),
                  ],
                ),
                IconButton(
                  onPressed: isConnected ? _exitDoodleOnBmo : null,
                  tooltip: 'Tutup di Layar BMO',
                  icon: const Icon(Icons.close_fullscreen_rounded, color: BmoTheme.bmoMint),
                ),
              ],
            ),
            const SizedBox(height: 16),

            // Live Canvas Frame (Rasio 128 x 64 = 2:1)
            GlassContainer(
              padding: const EdgeInsets.all(12),
              fill: const Color(0x35142238),
              borderColor: BmoTheme.bmoMint.withOpacity(0.5),
              child: Column(
                children: [
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      Row(
                        children: [
                          Container(
                            width: 8,
                            height: 8,
                            decoration: BoxDecoration(
                              shape: BoxShape.circle,
                              color: isConnected ? BmoTheme.statusOnline : BmoTheme.statusOffline,
                            ),
                          ),
                          const SizedBox(width: 6),
                          Text(
                            isConnected ? 'OLED 128x64 TERHUBUNG' : 'MODE OFFLINE',
                            style: TextStyle(
                              fontSize: 10,
                              fontWeight: FontWeight.bold,
                              letterSpacing: 0.8,
                              color: isConnected ? BmoTheme.statusOnline : BmoTheme.textMuted,
                            ),
                          ),
                        ],
                      ),
                      Text(
                        _isInverted ? 'INVERT (HITAM)' : 'NORMAL (PUTIH)',
                        style: const TextStyle(fontSize: 10, color: BmoTheme.textMuted),
                      ),
                    ],
                  ),
                  const SizedBox(height: 10),

                  // Area Gambar Interaktif
                  AspectRatio(
                    aspectRatio: 128.0 / 64.0,
                    child: Container(
                      decoration: BoxDecoration(
                        color: _isInverted ? const Color(0xFF111714) : const Color(0xFFCDE5D8), // Authentic BMO Mint Screen
                        borderRadius: BorderRadius.circular(16),
                        border: Border.all(
                          color: const Color(0xFF8BAE9D),
                          width: 3,
                        ),
                        boxShadow: [
                          BoxShadow(
                            color: BmoTheme.bmoMint.withOpacity(0.2),
                            blurRadius: 16,
                            spreadRadius: 2,
                          ),
                        ],
                      ),
                      child: ClipRRect(
                        borderRadius: BorderRadius.circular(13),
                        child: LayoutBuilder(
                          builder: (context, constraints) {
                            return GestureDetector(
                              onPanDown: (details) {
                                _handleTouch(details.localPosition, constraints.biggest, true);
                              },
                              onPanUpdate: (details) {
                                _handleTouch(details.localPosition, constraints.biggest, false);
                              },
                              onPanEnd: (_) => _onPanEnd(),
                              child: CustomPaint(
                                size: Size(constraints.maxWidth, constraints.maxHeight),
                                painter: _OledBitmapPainter(
                                  pixels: _pixels,
                                  isInverted: _isInverted,
                                ),
                              ),
                            );
                          },
                        ),
                      ),
                    ),
                  ),

                  const SizedBox(height: 10),
                  Text(
                    _statusText,
                    style: const TextStyle(
                      fontSize: 11,
                      fontStyle: FontStyle.italic,
                      color: BmoTheme.textMuted,
                    ),
                    textAlign: TextAlign.center,
                  ),
                ],
              ),
            ),
            const SizedBox(height: 16),

            // Toolbar Tools (Pencil, Eraser, Brush Size, Clear, Invert)
            GlassContainer(
              padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceAround,
                children: [
                  // Tool Pensil
                  _toolButton(
                    icon: Icons.edit_rounded,
                    label: 'Pensil',
                    isActive: !_isEraser,
                    onTap: () {
                      HapticFeedback.selectionClick();
                      setState(() => _isEraser = false);
                    },
                  ),
                  // Tool Penghapus
                  _toolButton(
                    icon: Icons.cleaning_services_rounded,
                    label: 'Hapus',
                    isActive: _isEraser,
                    onTap: () {
                      HapticFeedback.selectionClick();
                      setState(() => _isEraser = true);
                    },
                  ),
                  // Ukuran Kuas (1px / 2px)
                  _toolButton(
                    icon: _brushSize == 1 ? Icons.line_weight_rounded : Icons.brush_rounded,
                    label: _brushSize == 1 ? '1x Tipis' : '2x Tebal',
                    isActive: false,
                    onTap: () {
                      HapticFeedback.selectionClick();
                      setState(() => _brushSize = (_brushSize == 1) ? 2 : 1);
                    },
                  ),
                  // Invert Warna
                  _toolButton(
                    icon: Icons.invert_colors_rounded,
                    label: 'Invert',
                    isActive: _isInverted,
                    onTap: _toggleInvert,
                  ),
                  // Bersihkan Semua
                  _toolButton(
                    icon: Icons.delete_outline_rounded,
                    label: 'Clear',
                    isActive: false,
                    onTap: _clearCanvas,
                  ),
                ],
              ),
            ),
            const SizedBox(height: 16),

            // Preset Gambar Cepat
            GlassContainer(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'Gambar Cepat 1-Tap (Preset)',
                    style: TextStyle(fontSize: 14, fontWeight: FontWeight.w600, color: BmoTheme.textPrimary),
                  ),
                  const SizedBox(height: 4),
                  const Text(
                    'Pilih bentuk preset untuk langsung digambar ke BMO:',
                    style: TextStyle(fontSize: 12, color: BmoTheme.textSecondary),
                  ),
                  const SizedBox(height: 12),
                  Wrap(
                    spacing: 8,
                    runSpacing: 8,
                    children: [
                      _presetChip(
                        icon: Icons.favorite_rounded,
                        label: 'Hati',
                        onTap: () => _drawPreset('heart'),
                      ),
                      _presetChip(
                        icon: Icons.sentiment_very_satisfied_rounded,
                        label: 'Senyum',
                        onTap: () => _drawPreset('smile'),
                      ),
                      _presetChip(
                        icon: Icons.star_rounded,
                        label: 'Bintang',
                        onTap: () => _drawPreset('star'),
                      ),
                      _presetChip(
                        icon: Icons.pets_rounded,
                        label: 'Kucing',
                        onTap: () => _drawPreset('cat'),
                      ),
                      _presetChip(
                        icon: Icons.smart_toy_rounded,
                        label: 'BMO',
                        onTap: () => _drawPreset('bmo'),
                      ),
                      _presetChip(
                        icon: Icons.sports_esports_rounded,
                        label: 'Gamepad',
                        onTap: () => _drawPreset('gamepad'),
                      ),
                      _presetChip(
                        icon: Icons.bolt_rounded,
                        label: 'Petir',
                        onTap: () => _drawPreset('lightning'),
                      ),
                      _presetChip(
                        icon: Icons.music_note_rounded,
                        label: 'Musik',
                        onTap: () => _drawPreset('music'),
                      ),
                      _presetChip(
                        icon: Icons.dark_mode_rounded,
                        label: 'Bulan',
                        onTap: () => _drawPreset('moon'),
                      ),
                      _presetChip(
                        icon: Icons.coffee_rounded,
                        label: 'Kopi',
                        onTap: () => _drawPreset('coffee'),
                      ),
                      _presetChip(
                        icon: Icons.blur_on_rounded,
                        label: 'Hantu',
                        onTap: () => _drawPreset('ghost'),
                      ),
                      _presetChip(
                        icon: Icons.local_fire_department_rounded,
                        label: 'Api',
                        onTap: () => _drawPreset('fire'),
                      ),
                    ],
                  ),
                ],
              ),
            ),
            const SizedBox(height: 16),

            // Tombol Kirim Ulang Manual
            ElevatedButton.icon(
              onPressed: isConnected ? _sendFullBitmapToBmo : null,
              icon: const Icon(Icons.send_rounded, size: 18),
              label: const Text('Kirim Ulang Gambar Penuh ke Layar BMO'),
              style: ElevatedButton.styleFrom(
                backgroundColor: BmoTheme.primary,
                foregroundColor: Colors.white,
                minimumSize: const Size.fromHeight(48),
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
              ),
            ),
          ],
        );
      },
    );
  }

  Widget _toolButton({
    required IconData icon,
    required String label,
    required bool isActive,
    required VoidCallback onTap,
  }) {
    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(12),
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 180),
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
        decoration: BoxDecoration(
          color: isActive ? BmoTheme.bmoMint.withOpacity(0.25) : Colors.transparent,
          borderRadius: BorderRadius.circular(12),
          border: isActive ? Border.all(color: BmoTheme.bmoMint.withOpacity(0.6)) : null,
        ),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(
              icon,
              size: 20,
              color: isActive ? BmoTheme.bmoMint : BmoTheme.textSecondary,
            ),
            const SizedBox(height: 3),
            Text(
              label,
              style: TextStyle(
                fontSize: 10,
                fontWeight: isActive ? FontWeight.bold : FontWeight.normal,
                color: isActive ? Colors.white : BmoTheme.textMuted,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _presetChip({
    required IconData icon,
    required String label,
    required VoidCallback onTap,
  }) {
    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(10),
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 11, vertical: 7),
        decoration: BoxDecoration(
          color: BmoTheme.primary.withOpacity(0.2),
          borderRadius: BorderRadius.circular(10),
          border: Border.all(color: BmoTheme.primaryLight.withOpacity(0.35)),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(icon, size: 15, color: BmoTheme.bmoMint),
            const SizedBox(width: 6),
            Text(
              label,
              style: const TextStyle(
                fontSize: 12,
                fontWeight: FontWeight.w600,
                color: Colors.white,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class Point {
  final int x;
  final int y;
  Point(this.x, this.y);
}

class _OledBitmapPainter extends CustomPainter {
  final List<bool> pixels;
  final bool isInverted;

  _OledBitmapPainter({required this.pixels, required this.isInverted});

  @override
  void paint(Canvas canvas, Size size) {
    final scaleX = size.width / 128.0;
    final scaleY = size.height / 64.0;

    final paint = Paint()
      ..color = isInverted ? const Color(0xFFCDE5D8) : const Color(0xFF2C443B)
      ..style = PaintingStyle.fill;

    for (int y = 0; y < 64; y++) {
      for (int x = 0; x < 128; x++) {
        if (pixels[x + y * 128]) {
          final rect = Rect.fromLTWH(
            x * scaleX,
            y * scaleY,
            scaleX + 0.3,
            scaleY + 0.3,
          );
          canvas.drawRect(rect, paint);
        }
      }
    }
  }

  @override
  bool shouldRepaint(covariant _OledBitmapPainter oldDelegate) {
    return true;
  }
}
