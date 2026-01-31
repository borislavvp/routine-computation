import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

class BLEService {
  static final BLEService _instance = BLEService._internal();
  factory BLEService() => _instance;
  BLEService._internal();

  // UUIDs for the necklace device
  static const String SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
  static const String CAMERA_CHARACTERISTIC_UUID =
      "beb5483e-36e1-4688-b7f5-ea07361b26a8";
  static const String AUDIO_CHARACTERISTIC_UUID =
      "d2b5483e-36e1-4688-b7f5-ea07361b26aa";

  BluetoothDevice? _device;
  BluetoothCharacteristic? _commandCharacteristic;
  bool receivingImage = false;
  int expectedPackets = 0;
  int receivedPackets = 0;
  final List<int> _rxBuffer = [];
  final Map<int, Uint8List> packetBuffer = {};
  int imageSize = 0;
  final StreamController<Uint8List> _imageStreamController =
      StreamController<Uint8List>.broadcast();
  Stream<Uint8List> get imageStream => _imageStreamController.stream;

  final StreamController<Uint8List> _audioStreamController =
      StreamController<Uint8List>.broadcast();
  Stream<Uint8List> get audioStream => _audioStreamController.stream;

  bool _isConnected = false;
  bool get isConnected => _isConnected;

  Future<bool> requestPermissions() async {
    Map<Permission, PermissionStatus> statuses = await [
      Permission.bluetooth,
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.bluetoothAdvertise,
      Permission.location,
      Permission.storage
    ].request();

    return statuses.values.every((status) => status.isGranted);
  }

  Future<void> startScan() async {
    try {
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 4));
    } catch (e) {
      print('Error starting scan: $e');
    }
  }

  Future<void> stopScan() async {
    try {
      await FlutterBluePlus.stopScan();
    } catch (e) {
      print('Error stopping scan: $e');
    }
  }

  Stream<List<ScanResult>> get scanResults => FlutterBluePlus.scanResults;

  Uint8List _parseHexImageData(String hexString) {
    // Remove spaces and split into pairs
    final hexPairs = hexString.replaceAll(' ', '').split('');
    final bytes = <int>[];

    // Convert each pair of hex characters to a byte
    for (var i = 0; i < hexPairs.length; i += 2) {
      if (i + 1 < hexPairs.length) {
        final byte = int.parse(hexPairs[i] + hexPairs[i + 1], radix: 16);
        bytes.add(byte);
      }
    }

    return Uint8List.fromList(bytes);
  }

  Future<void> connect(BluetoothDevice device) async {
    try {
      _device = device;
      await device.connect();
      _isConnected = true;
      // Discover services
      List<BluetoothService> services = await device.discoverServices();
      for (var service in services) {
        for (var characteristic in service.characteristics) {
          if (characteristic.uuid.toString() == CAMERA_CHARACTERISTIC_UUID) {
            _commandCharacteristic = characteristic;
            await characteristic.setNotifyValue(true);
            characteristic.onValueReceived.listen((value) async {
              // Convert the received value to a hex string
              // print(value);
              if (value.isEmpty) return;

              // -------- HEADER --------
              if (!receivingImage &&
                  value.length == 8 &&
                  value[0] == 0xFF &&
                  value[1] == 0xFF) {
                final data = ByteData.sublistView(Uint8List.fromList(value));
                imageSize = data.getUint32(2, Endian.little);
                expectedPackets = data.getUint16(6, Endian.little);

                packetBuffer.clear();
                receivingImage = true;

                await characteristic.write(
                  utf8.encode("ACK:0"),
                  withoutResponse: true,
                );

                print(
                    "Image start: $imageSize bytes, $expectedPackets packets");
                return;
              }

              // -------- DATA --------
              if (receivingImage && value.length > 2) {
                final seq = (value[0] << 8) | value[1];
                final payload = Uint8List.fromList(value.sublist(2));

                packetBuffer[seq] = payload;
                final int ackN = seq + 1;
                print("SEND ACK");
                print(ackN);
                await characteristic.write(
                  utf8.encode("ACK:$ackN"),
                );
                bool allPacketsReceived = true;
                for (int i = 0; i < expectedPackets; i++) {
                  if (!packetBuffer.containsKey(i)) {
                    allPacketsReceived = false;
                    break;
                  }
                }
                if (allPacketsReceived) {
                  final builder = BytesBuilder();
                  for (int i = 0; i < expectedPackets; i++) {
                    builder.add(packetBuffer[i]!);
                  }

                  final imageBytes = builder.toBytes();
                  _imageStreamController.add( Uint8List.fromList([...imageBytes, 0xFF, 0xD9]));

                  receivingImage = false;
                  packetBuffer.clear();

                  print("Image complete");
                }
              }
            });
          } else if (characteristic.uuid.toString() ==
              AUDIO_CHARACTERISTIC_UUID) {
            characteristic.onValueReceived.listen((value) {
              _audioStreamController.add(Uint8List.fromList(value));
            });
          }
        }
      }
    } catch (e) {
      print('Error connecting to device: $e');
      rethrow;
    }
  }

  Future<void> disconnect() async {
    try {
      if (_device != null) {
        await _device!.disconnect();
        _isConnected = false;
        _device = null;
        _commandCharacteristic = null;
      }
    } catch (e) {
      print('Error disconnecting: $e');
    }
  }

  Future<void> sendCommand(String command) async {
    if (_commandCharacteristic == null) {
      throw Exception('Not connected to device');
    }
    try {
      await _commandCharacteristic!.write(utf8.encode(command));
    } catch (e) {
      print('Error sending command: $e');
      rethrow;
    }
  }

  Future<void> transferSdImages() async {
    await sendCommand('START_SD_TRANSFER');
  }

  Future<void> takePhoto() async {
    await sendCommand('START_CAMERA');
  }

  Future<void> startAudioRecording() async {
    await sendCommand('START_AUDIO');
  }

  Future<void> stopAudioRecording() async {
    await sendCommand('STOP_AUDIO');
  }

  void dispose() {
    _imageStreamController.close();
    _audioStreamController.close();
  }
}
