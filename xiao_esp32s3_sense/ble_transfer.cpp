#include "ble_transfer.h"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected");
    }
};

bool initBLE() {
  // Initialize BLE
  BLEDevice::init("ESP32-Camera");
  
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Add a descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE device ready to be connected");
  return true;
}

void sendImageViaBLE(camera_fb_t *fb) {
  if (!deviceConnected) {
    Serial.println("No device connected");
    return;
  }

  // Send image size first
  uint32_t imageSize = fb->len;
  pCharacteristic->setValue((uint8_t*)&imageSize, sizeof(imageSize));
  pCharacteristic->notify();
  delay(100);

  // Send image data in chunks
  const int chunkSize = 512;  // BLE packet size
  for (int i = 0; i < fb->len; i += chunkSize) {
    int currentChunkSize = min(chunkSize, (int)(fb->len - i));
    pCharacteristic->setValue(&fb->buf[i], currentChunkSize);
    pCharacteristic->notify();
    delay(20);  // Small delay between chunks
  }
  
  Serial.println("Image sent via BLE");
}

bool isDeviceConnected() {
  return deviceConnected;
} 