#ifndef IMU_HANDLER_H
#define IMU_HANDLER_H

#include <Arduino.h>
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050.h"

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

// Constants for MPU6050
#define ACC_RANGE           1 // 0: -/+2G; 1: +/-4G
#define CONVERT_G_TO_MS2    (9.81/(16384/(1.+ACC_RANGE)))
#define MAX_ACCEPTED_RANGE  (2*9.81)+(2*9.81)*ACC_RANGE

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