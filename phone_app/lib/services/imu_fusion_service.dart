import 'dart:async';
import 'dart:collection';
import 'dart:math';
import 'package:sensors_plus/sensors_plus.dart';

class IMUFusionService {
  static final IMUFusionService _instance = IMUFusionService._internal();
  factory IMUFusionService() => _instance;
  IMUFusionService._internal();

  // Stream controller for fused data
  final StreamController<FusedIMUData> _fusedDataController = StreamController<FusedIMUData>.broadcast();
  Stream<FusedIMUData> get fusedDataStream => _fusedDataController.stream;

  // Data buffers for sliding window
  final Queue<AccelerometerEvent> _accelBuffer = Queue<AccelerometerEvent>();
  final Queue<GyroscopeEvent> _gyroBuffer = Queue<GyroscopeEvent>();
  final Queue<MagnetometerEvent> _magBuffer = Queue<MagnetometerEvent>();

  // Window size in milliseconds
  int _windowSizeMs = 2000; // 2 seconds default
  Timer? _analysisTimer;
  bool _isAnalyzing = false;

  // Set window size
  void setWindowSize(int milliseconds) {
    if (milliseconds > 0) {
      _windowSizeMs = milliseconds;
      if (_isAnalyzing) {
        stopAnalysis();
        startAnalysis();
      }
    }
  }

  // Start analysis
  void startAnalysis() {
    if (_isAnalyzing) return;
    _isAnalyzing = true;

    _analysisTimer = Timer.periodic(const Duration(milliseconds: 100), (timer) {
      _analyzeData();
    });
  }

  // Stop analysis
  void stopAnalysis() {
    _analysisTimer?.cancel();
    _analysisTimer = null;
    _isAnalyzing = false;
  }

  // Add new sensor data
  void addAccelerometerData(AccelerometerEvent event) {
    _accelBuffer.add(event);
    _trimBuffer(_accelBuffer);
  }

  void addGyroscopeData(GyroscopeEvent event) {
    _gyroBuffer.add(event);
    _trimBuffer(_gyroBuffer);
  }

  void addMagnetometerData(MagnetometerEvent event) {
    _magBuffer.add(event);
    _trimBuffer(_magBuffer);
  }

  // Trim buffer to window size
  void _trimBuffer(Queue buffer) {
    // Keep only the last 20 samples (approximately 2 seconds at 100Hz)
    const int maxSamples = 20;
    while (buffer.length > maxSamples) {
      buffer.removeFirst();
    }
  }

  // Analyze data in the current window
  void _analyzeData() {
    if (_accelBuffer.isEmpty || _gyroBuffer.isEmpty) return;

    // Calculate features
    double accelVariance = _calculateAccelerationVariance();
    double gyroVariance = _calculateGyroscopeVariance();
    double motionMagnitude = _calculateMotionMagnitude();

    // Determine behavior state
    String behaviorState = _determineBehaviorState(
      accelVariance: accelVariance,
      gyroVariance: gyroVariance,
      motionMagnitude: motionMagnitude,
    );

    // Create fused data object
    final fusedData = FusedIMUData(
      timestamp: DateTime.now(),
      accelVariance: accelVariance,
      gyroVariance: gyroVariance,
      motionMagnitude: motionMagnitude,
      behaviorState: behaviorState,
    );

    // Emit fused data
    _fusedDataController.add(fusedData);
  }

  // Calculate acceleration variance
  double _calculateAccelerationVariance() {
    if (_accelBuffer.isEmpty) return 0.0;

    double sumX = 0, sumY = 0, sumZ = 0;
    double sumX2 = 0, sumY2 = 0, sumZ2 = 0;
    int count = _accelBuffer.length;

    for (var event in _accelBuffer) {
      sumX += event.x;
      sumY += event.y;
      sumZ += event.z;
      sumX2 += event.x * event.x;
      sumY2 += event.y * event.y;
      sumZ2 += event.z * event.z;
    }

    double meanX = sumX / count;
    double meanY = sumY / count;
    double meanZ = sumZ / count;

    return (sumX2 / count - meanX * meanX +
            sumY2 / count - meanY * meanY +
            sumZ2 / count - meanZ * meanZ) / 3;
  }

  // Calculate gyroscope variance
  double _calculateGyroscopeVariance() {
    if (_gyroBuffer.isEmpty) return 0.0;

    double sumX = 0, sumY = 0, sumZ = 0;
    double sumX2 = 0, sumY2 = 0, sumZ2 = 0;
    int count = _gyroBuffer.length;

    for (var event in _gyroBuffer) {
      sumX += event.x;
      sumY += event.y;
      sumZ += event.z;
      sumX2 += event.x * event.x;
      sumY2 += event.y * event.y;
      sumZ2 += event.z * event.z;
    }

    double meanX = sumX / count;
    double meanY = sumY / count;
    double meanZ = sumZ / count;

    return (sumX2 / count - meanX * meanX +
            sumY2 / count - meanY * meanY +
            sumZ2 / count - meanZ * meanZ) / 3;
  }

  // Calculate motion magnitude
  double _calculateMotionMagnitude() {
    if (_accelBuffer.isEmpty) return 0.0;

    double maxMagnitude = 0.0;
    for (var event in _accelBuffer) {
      double magnitude = sqrt(event.x * event.x + event.y * event.y + event.z * event.z);
      maxMagnitude = magnitude > maxMagnitude ? magnitude : maxMagnitude;
    }
    return maxMagnitude;
  }

  // Determine behavior state based on sensor data
  String _determineBehaviorState({
    required double accelVariance,
    required double gyroVariance,
    required double motionMagnitude,
  }) {
    // These thresholds should be calibrated based on real-world testing
    const double highMotionThreshold = 15.0;
    const double mediumMotionThreshold = 5.0;
    const double highVarianceThreshold = 10.0;
    const double mediumVarianceThreshold = 3.0;

    if (motionMagnitude > highMotionThreshold || 
        accelVariance > highVarianceThreshold || 
        gyroVariance > highVarianceThreshold) {
      return "walking_with_phone";
    } else if (motionMagnitude > mediumMotionThreshold || 
               accelVariance > mediumVarianceThreshold || 
               gyroVariance > mediumVarianceThreshold) {
      return "light_activity";
    } else {
      return "sitting_idle";
    }
  }

  // Dispose resources
  void dispose() {
    stopAnalysis();
    _fusedDataController.close();
    _accelBuffer.clear();
    _gyroBuffer.clear();
    _magBuffer.clear();
  }
}

// Data class for fused IMU data
class FusedIMUData {
  final DateTime timestamp;
  final double accelVariance;
  final double gyroVariance;
  final double motionMagnitude;
  final String behaviorState;

  FusedIMUData({
    required this.timestamp,
    required this.accelVariance,
    required this.gyroVariance,
    required this.motionMagnitude,
    required this.behaviorState,
  });
} 