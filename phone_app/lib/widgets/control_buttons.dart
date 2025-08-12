import 'package:flutter/material.dart';
import '../services/ble_service.dart';

class ControlButtons extends StatelessWidget {
  final BLEService bleService;

  const ControlButtons({
    Key? key,
    required this.bleService,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
      children: [
        // Camera button
        IconButton(
          icon: const Icon(Icons.camera_alt),
          iconSize: 48,
          onPressed: () async {
            try {
              await bleService.takePhoto();
            } catch (e) {
              ScaffoldMessenger.of(context).showSnackBar(
                SnackBar(content: Text('Error taking photo: $e')),
              );
            }
          },
          tooltip: 'Take Photo',
        ),
        // Start Audio button
        IconButton(
          icon: const Icon(Icons.mic),
          iconSize: 48,
          onPressed: () async {
            try {
              await bleService.startAudioRecording();
            } catch (e) {
              ScaffoldMessenger.of(context).showSnackBar(
                SnackBar(content: Text('Error starting audio: $e')),
              );
            }
          },
          tooltip: 'Start Recording',
        ),
        // Stop Audio button
        IconButton(
          icon: const Icon(Icons.stop),
          iconSize: 48,
          onPressed: () async {
            try {
              await bleService.stopAudioRecording();
            } catch (e) {
              ScaffoldMessenger.of(context).showSnackBar(
                SnackBar(content: Text('Error stopping audio: $e')),
              );
            }
          },
          tooltip: 'Stop Recording',
        ),
      ],
    );
  }
}
