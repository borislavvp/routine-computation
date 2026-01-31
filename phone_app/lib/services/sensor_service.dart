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
  final StreamController<MotionData> _motionController = StreamController<MotionData>.broadcast();

  // Public streams
  Stream<AccelerometerEvent> get accelerometerStream => _accelerometerController.stream;
  Stream<GyroscopeEvent> get gyroscopeStream => _gyroscopeController.stream;
  Stream<MagnetometerEvent> get magnetometerStream => _magnetometerController.stream;
  Stream<MotionData> get motionStream => _motionController.stream;

  // Stream subscriptions       
  StreamSubscription<AccelerometerEvent>? _accelerometerSubscription;
  StreamSubscription<GyroscopeEvent>? _gyroscopeSubscription;
  StreamSubscription<MagnetometerEvent>? _magnetometerSubscription;

  // Sampling rate (in Hz)
  int _samplingRate = 50; // Default to 50Hz
  bool _isSampling = false;

  // Motion data
  MotionData _currentMotion = MotionData();
  MotionData get currentMotion => _currentMotion;

  // Complementary filter coefficient (0-1)
  // Higher values give more weight to accelerometer data
  final double _alpha = 0.96;

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
        _updateAccelerometer(event);
      },
      onError: (error) {
        print('Error in accelerometer stream: $error');
      },
    );

    // Subscribe to gyroscope events
    _gyroscopeSubscription = gyroscopeEventStream().listen(
      (GyroscopeEvent event) {
        _gyroscopeController.add(event);
        _updateGyroscope(event);
      },
      onError: (error) {
        print('Error in gyroscope stream: $error');
      },
    );

    // Subscribe to magnetometer events
    _magnetometerSubscription = magnetometerEventStream().listen(
      (MagnetometerEvent event) {
        _magnetometerController.add(event);
        _updateMagnetometer(event);
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

  // Update accelerometer data and compute orientation
  void _updateAccelerometer(AccelerometerEvent event) {
    _currentMotion.accelerometer = Vector3(event.x, event.y, event.z);
    _computeOrientation();
  }

  // Update gyroscope data
  void _updateGyroscope(GyroscopeEvent event) {
    _currentMotion.gyroscope = Vector3(event.x, event.y, event.z);
  }

  // Update magnetometer data
  void _updateMagnetometer(MagnetometerEvent event) {
    _currentMotion.magnetometer = Vector3(event.x, event.y, event.z);
  }

  // Compute orientation using complementary filter
  void _computeOrientation() {
    // Get accelerometer data
    final accel = _currentMotion.accelerometer;
    
    // Compute pitch and roll from accelerometer
    double pitch = atan2(accel.y, sqrt(accel.x * accel.x + accel.z * accel.z));
    double roll = atan2(-accel.x, accel.z);

    // Apply complementary filter
    _currentMotion.orientation = Orientation(
      pitch: _alpha * pitch + (1 - _alpha) * _currentMotion.orientation.pitch,
      roll: _alpha * roll + (1 - _alpha) * _currentMotion.orientation.roll,
    );

    // Notify listeners
    _motionController.add(_currentMotion);
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
    _motionController.close();
  }
}

// Data classes for motion data
class Vector3 {
  final double x;
  final double y;
  final double z;

  Vector3(this.x, this.y, this.z);
}

class Orientation {
  final double pitch;
  final double roll;

  Orientation({this.pitch = 0.0, this.roll = 0.0});
}

class MotionData {
  Vector3 accelerometer = Vector3(0, 0, 0);
  Vector3 gyroscope = Vector3(0, 0, 0);
  Vector3 magnetometer = Vector3(0, 0, 0);
  Orientation orientation = Orientation();
} 