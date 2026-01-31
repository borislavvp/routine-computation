// #include "imu_handler.h"
// #include "ble_transfer.h"

// static MPU6050 imu;
// static bool recording = false;
// static IMUData recordedData[RECORD_BUFFER_SIZE];
// static int recordIndex = 0;
// static TaskHandle_t recordTaskHandle = NULL;

// bool initIMU() {
//     Wire.begin();
//     imu.initialize();
//     delay(10);

//     // Set MPU6050 Offset Calibration 
//     imu.setXAccelOffset(-4732);
//     imu.setYAccelOffset(4703);
//     imu.setZAccelOffset(8867);
//     imu.setXGyroOffset(61);
//     imu.setYGyroOffset(-73);
//     imu.setZGyroOffset(35);

//     imu.setFullScaleAccelRange(ACC_RANGE);

//     return true;
// }

// void readIMUData(IMUData* data) {
//     int16_t ax, ay, az;
//     int16_t gx, gy, gz;
    
//     imu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
//     // Convert to m/s² and rad/s
//     data->accelX = ax * CONVERT_G_TO_MS2;
//     data->accelY = ay * CONVERT_G_TO_MS2;
//     data->accelZ = az * CONVERT_G_TO_MS2;
    
//     // Convert to rad/s (MPU6050 sensitivity is 131 LSB/(deg/s))
//     data->gyroX = gx / 131.0f * (PI / 180.0f);
//     data->gyroY = gy / 131.0f * (PI / 180.0f);
//     data->gyroZ = gz / 131.0f * (PI / 180.0f);
    
//     data->timestamp = millis();
// }

// void recordTask(void *parameter) {
//     while (recording && recordIndex < RECORD_BUFFER_SIZE) {
//         readIMUData(&recordedData[recordIndex]);
//         recordIndex++;
//         delay(10); // 100Hz sampling rate
//     }
//     recording = false;
//     vTaskDelete(NULL);
// }

// void startRecording() {
//     if (!recording) {
//         recording = true;
//         recordIndex = 0;
//         xTaskCreate(recordTask, "RecordTask", 4096, NULL, 1, &recordTaskHandle);
//     }
// }

// void stopRecording() {
//     recording = false;
//     if (recordTaskHandle != NULL) {
//         vTaskDelete(recordTaskHandle);
//         recordTaskHandle = NULL;
//     }
// }

// bool isRecording() {
//     return recording;
// }

// void sendIMUDataViaBLE(const IMUData* data) {
//     // Send data via BLE
//     uint8_t buffer[28]; // 6 floats (4 bytes each) + 1 timestamp (4 bytes)
//     memcpy(buffer, &data->accelX, 4);
//     memcpy(buffer + 4, &data->accelY, 4);
//     memcpy(buffer + 8, &data->accelZ, 4);
//     memcpy(buffer + 12, &data->gyroX, 4);
//     memcpy(buffer + 16, &data->gyroY, 4);
//     memcpy(buffer + 20, &data->gyroZ, 4);
//     memcpy(buffer + 24, &data->timestamp, 4);
    
//     // Send via BLE characteristic
//     // TODO: Implement BLE sending
// }

// void sendRecordedDataViaBLE() {
//     for (int i = 0; i < recordIndex; i++) {
//         sendIMUDataViaBLE(&recordedData[i]);
//         delay(10); // Small delay to prevent BLE buffer overflow
//     }
// }

// void updateIMU() {
//     IMUData data;
//     readIMUData(&data);
//     sendIMUDataViaBLE(&data);
// } 