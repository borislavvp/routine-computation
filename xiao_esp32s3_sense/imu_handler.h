#ifndef IMU_HANDLER_H
#define IMU_HANDLER_H

#include <Arduino.h>
#include <Wire.h>
#include "Arduino_BMI270_BMM150.h"

// IMU data structure
struct IMUData {
    float accelX;
    float accelY;
    float accelZ;
    float gyroX;
    float gyroY;
    float gyroZ;
    unsigned long timestamp;
};

// Recording buffer size (10 seconds at 100Hz = 1000 samples)
#define RECORD_BUFFER_SIZE 1000

// Function declarations
bool initIMU();
void readIMUData(IMUData* data);
void startRecording();
void stopRecording();
bool isRecording();
void sendIMUDataViaBLE(const IMUData* data);
void sendRecordedDataViaBLE();
void updateIMU();

#endif 