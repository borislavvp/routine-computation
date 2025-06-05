#include "camera_config.h"
#include "sd_card.h"
#include "ble_transfer.h"
#include "imu_handler.h"

unsigned long lastCaptureTime = 0; // Last shooting time
int imageCount = 1;                // File Counter
bool camera_sign = false;          // Check camera status
bool sd_sign = false;              // Check sd status
bool commandRecv = false;          // flag used for indicating receipt of commands from serial port
bool captureFlag = false;
bool imu_sign = false;             // Check IMU status

void setup() {
  Serial.begin(115200);
  while(!Serial); // When the serial monitor is turned on, the program starts to execute

  // Initialize camera
  camera_sign = initCamera();
  if (!camera_sign) {
    Serial.println("Camera initialization failed");
    return;
  }

  // Initialize SD card
  sd_sign = initSDCard();
  if (!sd_sign) {
    Serial.println("SD card initialization failed");
    return;
  }

  // Initialize IMU
  imu_sign = initIMU();
  if (!imu_sign) {
    Serial.println("IMU initialization failed");
    return;
  }

  // Initialize BLE
  if (!initBLE()) {
    Serial.println("BLE initialization failed");
    return;
  }

  // List files in root directory
  Serial.println("Files in root directory:");
  listFiles(SD, "/");

  Serial.println("XIAO ESP32S3 Sense Camera Image Capture");
  Serial.println("Send 'capture' to initiates an image capture");
  Serial.println("Send 'record' to start recording IMU data for 10 seconds\n");
}

void loop() {
  // Update IMU data
  if (imu_sign) {
    updateIMU();
  }

  // Camera & SD available, start taking pictures
  if(camera_sign && sd_sign) {
    String command;
    // Read incoming commands from serial monitor
    while (Serial.available()) {
      char c = Serial.read();
      if ((c != '\n') && (c != '\r')) {
        command.concat(c);
      } 
      else if (c == '\r') {
        commandRecv = true;
        command.toLowerCase();
      }
    }
     
    // Handle commands
    if (commandRecv) {
      commandRecv = false;
      
      if (command == "capture") {
        Serial.println("\nPicture Capture Command is sent");
        
        // Capture photo
        camera_fb_t *fb = NULL;
        capturePhoto(&fb);
        
        if (fb) {
          // Save to SD card
          char filename[32];
          sprintf(filename, "/image%d.jpg", imageCount);
          writeFile(SD, filename, fb->buf, fb->len);
          Serial.printf("Saved picture：%s\n", filename);
          
          // Send via BLE if device is connected
          if (isDeviceConnected()) {
            sendImageViaBLE(fb);
          }
          
          esp_camera_fb_return(fb);
          imageCount++;
        }
        Serial.println("");
      }
      else if (command == "record" && imu_sign) {
        Serial.println("\nStarting IMU recording for 10 seconds");
        startRecording();
      }
    }
  }
} 