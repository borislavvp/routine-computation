import 'dart:typed_data';
import 'package:flutter/material.dart';

class ImageDisplay extends StatelessWidget {
  final Uint8List imageData;

  const ImageDisplay({Key? key, required this.imageData}) : super(key: key);

  // Uint8List _hexToBytes() {
  //   final cleaned = imageData.replaceAll(RegExp(r'[^0-9a-fA-F]'), '');
  //   final length = cleaned.length;
  //   final bytes = Uint8List(length ~/ 2);
  //   for (int i = 0; i < length; i += 2) {
  //     bytes[i ~/ 2] = int.parse(cleaned.substring(i, i + 2), radix: 16);
  //   }
  //   return bytes;
  // }

  @override
  Widget build(BuildContext context) {
    // final Uint8List imageBytes = _hexToBytes();

    return Image.memory(
      imageData,
      fit: BoxFit.contain,
      height: 100,
      width: 200,
      errorBuilder: (context, error, stackTrace) {
        return const Text('Failed to load image');
      },
    );
  }
}
