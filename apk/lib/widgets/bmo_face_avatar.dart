import 'package:flutter/material.dart';
import '../theme/bmo_theme.dart';

class BmoFaceAvatar extends StatelessWidget {
  final String mood;
  final double width;
  final double height;

  const BmoFaceAvatar({
    super.key,
    this.mood = 'happy',
    this.width = 160,
    this.height = 120,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      width: width,
      height: height,
      decoration: BoxDecoration(
        color: const Color(0xFFCDE5D8), // Authentic BMO Screen Mint
        borderRadius: BorderRadius.circular(18),
        border: Border.all(
          color: const Color(0xFF8BAE9D),
          width: 3,
        ),
        boxShadow: [
          BoxShadow(
            color: BmoTheme.bmoMint.withOpacity(0.25),
            blurRadius: 16,
            spreadRadius: 2,
          ),
        ],
      ),
      child: Stack(
        alignment: Alignment.center,
        children: [
          // Cheeks (Blush)
          if (mood == 'love' || mood == 'happy' || mood == 'hold' || mood == 'wink') ...[
            const Positioned(
              left: 20,
              bottom: 30,
              child: _BlushDot(),
            ),
            const Positioned(
              right: 20,
              bottom: 30,
              child: _BlushDot(),
            ),
          ],

          // Eyes
          Positioned(
            top: height * 0.32,
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                _buildEye(true),
                const SizedBox(width: 44),
                _buildEye(false),
              ],
            ),
          ),

          // Mouth
          Positioned(
            bottom: height * 0.22,
            child: _buildMouth(),
          ),
        ],
      ),
    );
  }

  Widget _buildEye(bool isLeft) {
    if (mood == 'wink' && isLeft) {
      // Wink eye (smile curve)
      return Container(
        width: 14,
        height: 6,
        decoration: BoxDecoration(
          border: Border(
            bottom: BorderSide(color: Colors.black.withOpacity(0.85), width: 3),
          ),
        ),
      );
    }

    if (mood == 'love') {
      return const Text('❤️', style: TextStyle(fontSize: 16));
    }

    if (mood == 'sleepy') {
      return Container(
        width: 14,
        height: 4,
        decoration: BoxDecoration(
          color: Colors.black.withOpacity(0.85),
          borderRadius: BorderRadius.circular(2),
        ),
      );
    }

    // Default lively eye
    return Container(
      width: 10,
      height: 18,
      decoration: BoxDecoration(
        color: Colors.black.withOpacity(0.88),
        borderRadius: BorderRadius.circular(6),
      ),
      child: Align(
        alignment: Alignment.topRight,
        child: Container(
          width: 3,
          height: 3,
          margin: const EdgeInsets.only(top: 2, right: 2),
          decoration: const BoxDecoration(
            color: Colors.white,
            shape: BoxShape.circle,
          ),
        ),
      ),
    );
  }

  Widget _buildMouth() {
    if (mood == 'happy' || mood == 'love' || mood == 'wink' || mood == 'tickle') {
      return Container(
        width: 26,
        height: 14,
        decoration: const BoxDecoration(
          color: Color(0xFF2C443B),
          borderRadius: BorderRadius.vertical(
            bottom: Radius.circular(16),
          ),
        ),
      );
    } else if (mood == 'sad' || mood == 'angry') {
      return Container(
        width: 20,
        height: 10,
        decoration: const BoxDecoration(
          color: Color(0xFF2C443B),
          borderRadius: BorderRadius.vertical(
            top: Radius.circular(14),
          ),
        ),
      );
    } else if (mood == 'surprise') {
      return Container(
        width: 14,
        height: 16,
        decoration: BoxDecoration(
          color: const Color(0xFF2C443B),
          borderRadius: BorderRadius.circular(10),
        ),
      );
    }

    // Default gentle smile
    return Container(
      width: 18,
      height: 4,
      decoration: BoxDecoration(
        color: const Color(0xFF2C443B),
        borderRadius: BorderRadius.circular(2),
      ),
    );
  }
}

class _BlushDot extends StatelessWidget {
  const _BlushDot();

  @override
  Widget build(BuildContext context) {
    return Container(
      width: 12,
      height: 7,
      decoration: BoxDecoration(
        color: const Color(0xFFF9A8A8).withOpacity(0.85),
        borderRadius: BorderRadius.circular(4),
      ),
    );
  }
}
