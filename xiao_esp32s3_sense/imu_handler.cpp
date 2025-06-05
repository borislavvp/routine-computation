#include "imu_handler.h"
#include "ble_transfer.h"

IMUData recordBuffer[RECORD_BUFFER_SIZE];
int recordIndex = 0;
bool recording = false;
unsigned long lastIMURead = 0;
const int IMU_READ_INTERVAL = 10; // 100Hz sampling rate

bool initIMU() {
    Wire.begin();
    if (!IMU.begin()) {
        Serial.println("Failed to find BMI270 chip");
        return false;
    }

    // Configure BMI270
    IMU.setAccelRange(BMI270_ACCEL_RANGE_2G);
    IMU.setGyroRange(BMI270_GYRO_RANGE_250);
    IMU.setAccelODR(BMI270_ACCEL_ODR_100HZ);
    IMU.setGyroODR(BMI270_GYRO_ODR_100HZ);
    
    Serial.println("BMI270 initialized successfully");
    return true;
}

void readIMUData(IMUData* data) {
    float ax, ay, az, gx, gy, gz;
    
    // Read accelerometer data
    IMU.readAcceleration(ax, ay, az);
    data->accelX = ax;
    data->accelY = ay;
    data->accelZ = az;
    
    // Read gyroscope data
    IMU.readGyroscope(gx, gy, gz);
    data->gyroX = gx;
    data->gyroY = gy;
    data->gyroZ = gz;
    
    data->timestamp = millis();
}

void startRecording() {
    recording = true;
    recordIndex = 0;
    Serial.println("Started recording IMU data");
}

void stopRecording() {
    recording = false;
    Serial.println("Stopped recording IMU data");
}

bool isRecording() {
    return recording;
}

void sendIMUDataViaBLE(const IMUData* data) {
    if (!isDeviceConnected()) return;
    
    // Create a buffer for the IMU data
    uint8_t buffer[sizeof(IMUData)];
    memcpy(buffer, data, sizeof(IMUData));
    
    // Send the data via BLE
    sendDataViaBLE(buffer, sizeof(IMUData));
}

void sendRecordedDataViaBLE() {
    if (!isDeviceConnected()) return;
    
    // Send each recorded data point
    for (int i = 0; i < recordIndex; i++) {
        sendIMUDataViaBLE(&recordBuffer[i]);
        delay(10); // Small delay to prevent BLE buffer overflow
    }
    
    Serial.printf("Sent %d IMU data points\n", recordIndex);
}

void updateIMU() {
    unsigned long currentTime = millis();
    if (currentTime - lastIMURead >= IMU_READ_INTERVAL) {
        lastIMURead = currentTime;
        
        IMUData currentData;
        readIMUData(&currentData);
        
        // If recording, store the data
        if (recording && recordIndex < RECORD_BUFFER_SIZE) {
            recordBuffer[recordIndex++] = currentData;
            
            // If buffer is full, stop recording
            if (recordIndex >= RECORD_BUFFER_SIZE) {
                stopRecording();
                sendRecordedDataViaBLE();
            }
        }
        
        // Always send current IMU data
        sendIMUDataViaBLE(&currentData);
    }
} 