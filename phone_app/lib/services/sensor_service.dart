import 'dart:async';
import 'dart:math';
import 'package:sensors_plus/sensors_plus.dart';

class SensorService {
  static final SensorService _instance = SensorService._internal();
  factory SensorService() => _instance;
  SensorService._internal();

  // Stream controllers for sensor data
  final StreamController<AccelerometerEvent> _accelerometerController = StreamController<AccelerometerEvent>.broadcast();
  final StreamController<GyroscopeEvent> _gyroscopeController = StreamController<GyroscopeEvent>.broadcast();
  final StreamController<MagnetometerEvent> _magnetometerController = StreamController<MagnetometerEvent>.broadcast();

  // Public streams
  Stream<AccelerometerEvent> get accelerometerStream => _accelerometerController.stream;
  Stream<GyroscopeEvent> get gyroscopeStream => _gyroscopeController.stream;
  Stream<MagnetometerEvent> get magnetometerStream => _magnetometerController.stream;

  // Stream subscriptions
  StreamSubscription<AccelerometerEvent>? _accelerometerSubscription;
  StreamSubscription<GyroscopeEvent>? _gyroscopeSubscription;
  StreamSubscription<MagnetometerEvent>? _magnetometerSubscription;

  // Sampling rate (in Hz)
  int _samplingRate = 50; // Default to 50Hz
  bool _isSampling = false;

  // Set sampling rate
  void setSamplingRate(int rateHz) {
    if (rateHz > 0) {
      _samplingRate = rateHz;
      if (_isSampling) {
        stopSampling();
        startSampling();
      }
    }
  }

  // Start sampling sensors
  void startSampling() {
    if (_isSampling) return;
    _isSampling = true;

    // Subscribe to accelerometer events
    _accelerometerSubscription = accelerometerEventStream().listen(
      (AccelerometerEvent event) {
        _accelerometerController.add(event);
      },
      onError: (error) {
        print('Error in accelerometer stream: $error');
      },
    );

    // Subscribe to gyroscope events
    _gyroscopeSubscription = gyroscopeEventStream().listen(
      (GyroscopeEvent event) {
        _gyroscopeController.add(event);
      },
      onError: (error) {
        print('Error in gyroscope stream: $error');
      },
    );

    // Subscribe to magnetometer events
    _magnetometerSubscription = magnetometerEventStream().listen(
      (MagnetometerEvent event) {
        _magnetometerController.add(event);
      },
      onError: (error) {
        print('Error in magnetometer stream: $error');
      },
    );
  }

  // Stop sampling sensors
  void stopSampling() {
    _accelerometerSubscription?.cancel();
    _gyroscopeSubscription?.cancel();
    _magnetometerSubscription?.cancel();
    
    _accelerometerSubscription = null;
    _gyroscopeSubscription = null;
    _magnetometerSubscription = null;
    
    _isSampling = false;
  }

  // Calculate acceleration magnitude
  double calculateAccelerationMagnitude(AccelerometerEvent event) {
    return sqrt(event.x * event.x + event.y * event.y + event.z * event.z);
  }

  // Calculate angular velocity magnitude
  double calculateAngularVelocityMagnitude(GyroscopeEvent event) {
    return sqrt(event.x * event.x + event.y * event.y + event.z * event.z);
  }

  // Dispose resources
  void dispose() {
    stopSampling();
    _accelerometerController.close();
    _gyroscopeController.close();
    _magnetometerController.close();
  }
} 