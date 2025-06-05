import 'dart:async';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

class BLEService {
  static final BLEService _instance = BLEService._internal();
  factory BLEService() => _instance;
  BLEService._internal();

  // UUIDs for the necklace device
  static const String SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
  static const String IMAGE_CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
  static const String IMU_CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

  StreamSubscription? _scanSubscription;
  StreamSubscription? _connectionSubscription;
  StreamSubscription? _imuSubscription;

  Future<bool> requestPermissions() async {
    Map<Permission, PermissionStatus> statuses = await [
      Permission.bluetooth,
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.bluetoothAdvertise,
      Permission.location,
    ].request();

    return statuses.values.every((status) => status.isGranted);
  }

  Future<void> startScan({Duration timeout = const Duration(seconds: 4)}) async {
    try {
      // Wait for Bluetooth to be on
      await FlutterBluePlus.adapterState
          .where((state) => state == BluetoothAdapterState.on)
          .first;

      // Start scanning
      await FlutterBluePlus.startScan(
        timeout: timeout,
        // Add your service UUID here if you want to filter
        // withServices: [Guid(SERVICE_UUID)],
      );

      // Listen to scan results
      _scanSubscription = FlutterBluePlus.scanResults.listen((results) {
        // Handle scan results
      }, onError: (e) {
        print('Error scanning: $e');
      });

    } catch (e) {
      print('Error starting scan: $e');
      rethrow;
    }
  }

  Future<void> stopScan() async {
    try {
      await FlutterBluePlus.stopScan();
      await _scanSubscription?.cancel();
    } catch (e) {
      print('Error stopping scan: $e');
      rethrow;
    }
  }

  Future<void> connectToDevice(BluetoothDevice device) async {
    try {
      // Listen for disconnection
      _connectionSubscription = device.connectionState.listen((state) async {
        if (state == BluetoothConnectionState.disconnected) {
          print("Disconnected: ${device.disconnectReason?.code} ${device.disconnectReason?.description}");
          // Handle disconnection
        }
      });

      // Connect to device
      await device.connect();

      


    } catch (e) {
      print('Error connecting to device: $e');
      rethrow;
    }
  }

  Future<void> _setupBluetoothCharacteristics(BluetoothDevice device) async {
// Discover services
      List<BluetoothService> services = await device.discoverServices();
      
      // Find our service
      BluetoothService? targetService = services.firstWhere(
        (service) => service.uuid.toString() == SERVICE_UUID,
        orElse: () => throw Exception('Service not found'),
      );

      // Set up notifications for IMU data
      BluetoothCharacteristic? imuCharacteristic = targetService.characteristics.firstWhere(
        (c) => c.uuid.toString() == IMU_CHARACTERISTIC_UUID,
        orElse: () => throw Exception('IMU characteristic not found'),
      );
      // Listen for IMU data
      _imuSubscription = imuCharacteristic.onValueReceived.listen((value) {
        _handleIMUData(value);
      });
      // Enable notifications
      await imuCharacteristic.setNotifyValue(true);
  }

  void _handleIMUData(List<int> value) {
    // TODO: Implement IMU data handling
    // This will be implemented when we create the IMU data model
  }

  Future<void> disconnectDevice(BluetoothDevice device) async {
    try {
      await device.disconnect();
      await _connectionSubscription?.cancel();
      await _imuSubscription?.cancel();
    } catch (e) {
      print('Error disconnecting from device: $e');
      rethrow;
    }
  }

  void dispose() {
    _scanSubscription?.cancel();
    _connectionSubscription?.cancel();
    _imuSubscription?.cancel();
  }
} 