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
    return SizedBox(
      width: width,
      height: height,
      child: FittedBox(
        fit: BoxFit.contain,
        alignment: Alignment.center,
        child: Container(
          width: 160,
          height: 120,
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
              if (mood == 'love' ||
                  mood == 'happy' ||
                  mood == 'hold' ||
                  mood == 'wink' ||
                  mood == 'laugh' ||
                  mood == 'excited' ||
                  mood == 'hungry') ...[
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
                top: 38,
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
                bottom: 24,
                child: _buildMouth(),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildEye(bool isLeft) {
    if (mood == 'cool') {
      return Container(
        width: 18,
        height: 12,
        decoration: BoxDecoration(
          color: Colors.black.withOpacity(0.9),
          borderRadius: BorderRadius.circular(4),
        ),
      );
    }

    if (mood == 'wink' && isLeft) {
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

    if (mood == 'laugh') {
      return Container(
        width: 14,
        height: 8,
        decoration: BoxDecoration(
          border: Border(
            top: BorderSide(color: Colors.black.withOpacity(0.88), width: 3),
          ),
        ),
      );
    }

    if (mood == 'love') {
      return const Icon(
        Icons.favorite_rounded,
        size: 16,
        color: Color(0xFFE91E63),
      );
    }

    if (mood == 'dizzy') {
      return Icon(
        Icons.close_rounded,
        size: 16,
        color: Colors.black.withOpacity(0.85),
      );
    }

    if (mood == 'glitch') {
      return Container(
        width: 12,
        height: 14,
        decoration: BoxDecoration(
          color: isLeft ? const Color(0xFF2C443B) : Colors.black87,
          borderRadius: BorderRadius.circular(2),
        ),
      );
    }

    if (mood == 'sleepy' || mood == 'blink') {
      return Container(
        width: 14,
        height: 4,
        decoration: BoxDecoration(
          color: Colors.black.withOpacity(0.85),
          borderRadius: BorderRadius.circular(2),
        ),
      );
    }

    if (mood == 'bored') {
      return Container(
        width: 12,
        height: 6,
        decoration: BoxDecoration(
          color: Colors.black.withOpacity(0.85),
          borderRadius: const BorderRadius.vertical(bottom: Radius.circular(6)),
        ),
      );
    }

    if (mood == 'surprise' || mood == 'scared') {
      return Container(
        width: 16,
        height: 16,
        decoration: BoxDecoration(
          shape: BoxShape.circle,
          border: Border.all(color: Colors.black.withOpacity(0.88), width: 3),
        ),
        child: Center(
          child: Container(
            width: 4,
            height: 4,
            decoration: const BoxDecoration(
              color: Colors.black,
              shape: BoxShape.circle,
            ),
          ),
        ),
      );
    }

    // Default lively eye with symmetrical highlights
    return Container(
      width: 10,
      height: 18,
      decoration: BoxDecoration(
        color: Colors.black.withOpacity(0.88),
        borderRadius: BorderRadius.circular(6),
      ),
      child: Align(
        alignment: isLeft ? Alignment.topLeft : Alignment.topRight,
        child: Container(
          width: 3,
          height: 3,
          margin: EdgeInsets.only(
            top: 2,
            left: isLeft ? 2 : 0,
            right: isLeft ? 0 : 2,
          ),
          decoration: const BoxDecoration(
            color: Colors.white,
            shape: BoxShape.circle,
          ),
        ),
      ),
    );
  }

  Widget _buildMouth() {
    if (mood == 'happy' ||
        mood == 'love' ||
        mood == 'wink' ||
        mood == 'tickle' ||
        mood == 'laugh' ||
        mood == 'excited' ||
        mood == 'wake') {
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
    } else if (mood == 'hungry') {
      return Stack(
        alignment: Alignment.bottomCenter,
        children: [
          Container(
            width: 24,
            height: 13,
            decoration: const BoxDecoration(
              color: Color(0xFF2C443B),
              borderRadius: BorderRadius.vertical(bottom: Radius.circular(14)),
            ),
          ),
          Container(
            width: 10,
            height: 6,
            decoration: const BoxDecoration(
              color: Color(0xFFFF8B94),
              borderRadius: BorderRadius.vertical(bottom: Radius.circular(5)),
            ),
          ),
        ],
      );
    } else if (mood == 'talk') {
      return Container(
        width: 14,
        height: 14,
        decoration: BoxDecoration(
          color: const Color(0xFF2C443B),
          borderRadius: BorderRadius.circular(7),
        ),
      );
    } else if (mood == 'sad' || mood == 'angry' || mood == 'cold') {
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
    } else if (mood == 'surprise' || mood == 'scared') {
      return Container(
        width: 14,
        height: 16,
        decoration: BoxDecoration(
          color: const Color(0xFF2C443B),
          borderRadius: BorderRadius.circular(10),
        ),
      );
    } else if (mood == 'dizzy' || mood == 'sick' || mood == 'glitch') {
      return Container(
        width: 20,
        height: 3,
        decoration: BoxDecoration(
          color: const Color(0xFF2C443B),
          borderRadius: BorderRadius.circular(2),
        ),
        transform: Matrix4.rotationZ(0.1),
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
