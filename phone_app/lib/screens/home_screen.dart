import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import '../services/ble_service.dart';
import '../services/sensor_service.dart';
import '../services/imu_fusion_service.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  final BLEService _bleService = BLEService();
  final SensorService _sensorService = SensorService();
  final IMUFusionService _imuFusionService = IMUFusionService();
  
  bool _isScanning = false;
  bool _isConnected = false;
  String _behaviorState = "Not connected";
  List<ScanResult> _scanResults = [];
  BluetoothDevice? _connectedDevice;

  @override
  void initState() {
    super.initState();
    _initializeServices();
  }

  Future<void> _initializeServices() async {
    // Request permissions
    bool permissionsGranted = await _bleService.requestPermissions();
    if (!permissionsGranted) {
      _showError("Bluetooth permissions not granted");
      return;
    }

    // Start sensor services
    _sensorService.startSampling();
    _imuFusionService.startAnalysis();

    // Set up sensor data listeners
    _sensorService.accelerometerStream.listen((event) {
      _imuFusionService.addAccelerometerData(event);
    });

    _sensorService.gyroscopeStream.listen((event) {
      _imuFusionService.addGyroscopeData(event);
    });

    _sensorService.magnetometerStream.listen((event) {
      _imuFusionService.addMagnetometerData(event);
    });

    // Listen for fused IMU data
    _imuFusionService.fusedDataStream.listen((data) {
      setState(() {
        _behaviorState = data.behaviorState;
      });
    });
  }

  void _showError(String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(message)),
    );
  }

  Future<void> _startScan() async {
    setState(() {
      _isScanning = true;
      _scanResults = [];
    });

    try {
      await _bleService.startScan();
      // Listen to scan results
      FlutterBluePlus.scanResults.listen((results) {
        setState(() {
          _scanResults = results;
        });
      }, onError: (e) {
        _showError("Error scanning: $e");
      });
    } catch (e) {
      _showError("Error starting scan: $e");
    }

    setState(() {
      _isScanning = false;
    });
  }

  Future<void> _connectToDevice(BluetoothDevice device) async {
    try {
      await _bleService.connectToDevice(device);
      setState(() {
        _isConnected = true;
        _connectedDevice = device;
      });
    } catch (e) {
      _showError("Error connecting: $e");
    }
  }

  Future<void> _disconnectDevice() async {
    if (_connectedDevice != null) {
      try {
        await _bleService.disconnectDevice(_connectedDevice!);
        setState(() {
          _isConnected = false;
          _connectedDevice = null;
        });
      } catch (e) {
        _showError("Error disconnecting: $e");
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Necklace App'),
        actions: [
          if (_isConnected)
            IconButton(
              icon: const Icon(Icons.bluetooth_disabled),
              onPressed: _disconnectDevice,
            ),
          IconButton(
            icon: Icon(_isScanning ? Icons.stop : Icons.search),
            onPressed: _isScanning ? _bleService.stopScan : _startScan,
          ),
        ],
      ),
      body: Column(
        children: [
          // Connection Status
          Container(
            padding: const EdgeInsets.all(16),
            color: _isConnected ? Colors.green[100] : Colors.red[100],
            child: Row(
              children: [
                Icon(
                  _isConnected ? Icons.bluetooth_connected : Icons.bluetooth_disabled,
                  color: _isConnected ? Colors.green : Colors.red,
                ),
                const SizedBox(width: 8),
                Text(
                  _isConnected ? 'Connected to ${_connectedDevice?.name ?? "Unknown"}' : 'Disconnected',
                  style: TextStyle(
                    color: _isConnected ? Colors.green[900] : Colors.red[900],
                  ),
                ),
              ],
            ),
          ),

          // Behavior State
          Container(
            padding: const EdgeInsets.all(16),
            child: Column(
              children: [
                const Text(
                  'Current Behavior State:',
                  style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
                ),
                const SizedBox(height: 8),
                Text(
                  _behaviorState,
                  style: const TextStyle(fontSize: 24),
                ),
              ],
            ),
          ),

          // Device List
          Expanded(
            child: ListView.builder(
              itemCount: _scanResults.length,
              itemBuilder: (context, index) {
                final result = _scanResults[index];
                final device = result.device;
                final name = result.advertisementData.advName.isEmpty
                    ? 'Unknown Device'
                    : result.advertisementData.advName;
                
                return ListTile(
                  title: Text(name),
                  subtitle: Text(device.remoteId.str),
                  trailing: ElevatedButton(
                    onPressed: _isConnected
                        ? null
                        : () => _connectToDevice(device),
                    child: const Text('Connect'),
                  ),
                );
              },
            ),
          ),
        ],
      ),
    );
  }

  @override
  void dispose() {
    _bleService.dispose();
    _sensorService.dispose();
    _imuFusionService.dispose();
    super.dispose();
  }
} 