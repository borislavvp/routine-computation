import 'dart:io';
import 'package:flutter/material.dart';

class ImageDisplay extends StatefulWidget {
  final String imagePath;
  final Duration retryDelay;
  final int maxRetries;

  const ImageDisplay({
    Key? key,
    required this.imagePath,
    this.retryDelay = const Duration(milliseconds: 200),
    this.maxRetries = 10,
  }) : super(key: key);

  @override
  State<ImageDisplay> createState() => _ImageDisplayState();
}

class _ImageDisplayState extends State<ImageDisplay> {
  int _attempt = 0;
  late FileImage _fileImage;

  @override
  void initState() {
    super.initState();
    _attempt = 0;
    _loadImage();
  }

  void _loadImage() {
    print('[ImageDisplay] Attempt $_attempt to load ${widget.imagePath}');
    // Always create a new FileImage instance
    _fileImage = FileImage(File(widget.imagePath));

    // Evict cache for fresh load
    PaintingBinding.instance.imageCache.evict(_fileImage);

    _fileImage.resolve(const ImageConfiguration()).addListener(
          ImageStreamListener(
            (image, synchronousCall) {
              print('[ImageDisplay] Image loaded successfully!');
            },
            onError: (error, stackTrace) async {
              print('[ImageDisplay] Failed to load image: $error');
              if (_attempt < widget.maxRetries) {
                _attempt++;
                await Future.delayed(widget.retryDelay);
                if (mounted) setState(_loadImage);
              } else {
                print('[ImageDisplay] Max retries reached, giving up.');
              }
            },
          ),
        );
  }

  @override
  void didUpdateWidget(covariant ImageDisplay oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.imagePath != widget.imagePath) {
      _attempt = 0;
      _loadImage();
    }
  }

  @override
  Widget build(BuildContext context) {
    // Key changes with _attempt to force rebuild
    return Image(
      key: ValueKey('${widget.imagePath}_$_attempt'),
      image: _fileImage,
      width: 200,
      height: 100,
      fit: BoxFit.contain,
      errorBuilder: (context, error, stackTrace) {
        return Container(
          width: 200,
          height: 100,
          color: Colors.grey[300],
          child: const Center(
            child: Icon(Icons.broken_image, size: 48, color: Colors.grey),
          ),
        );
      },
    );
  }
}
