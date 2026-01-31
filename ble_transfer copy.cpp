#include "ble_transfer.h"
#include "audio_handler.h"
#include "camera_config.h"
#include "sd_card.h"

BLEServer *pServer = NULL;
BLECharacteristic *pCameraCharacteristic = NULL;
BLECharacteristic *pAudioCharacteristic = NULL;
bool deviceConnected = false;

void blink()
{ // Try both polarities
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  Serial.println("Blink test");
}

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    blink();
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    blink();
    // Restart advertising so clients can reconnect
    BLEDevice::startAdvertising();
    Serial.println("Restarted advertising");
    blink();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    String value = pCharacteristic->getValue().c_str();

    if (value == "START_CAMERA")
    {
      // Capture photo
      camera_fb_t *fb = capturePhoto();
      if (fb)
      {
        // Save to SD card with timestamp
        char filename[32];
        time_t now = time(nullptr);
        strftime(filename, sizeof(filename), "/photo_%Y%m%d_%H%M%S.jpg", localtime(&now));
        savePhoto(filename, fb);

        // Send via BLE
        sendImageViaBLE(fb);

        // Free the frame buffer
        esp_camera_fb_return(fb);
      }
    }
    else if (value == "START_AUDIO")
    {
      startRecording();
    }
    else if (value == "STOP_AUDIO")
    {
      stopRecording();
      // Get the recorded audio data
      uint8_t *audioData = getAudioData();
      size_t audioSize = getAudioDataSize();
      if (audioData && audioSize > 0)
      {
        // Send the audio data via BLE
        sendAudioViaBLE(audioData, audioSize);
      }
    }
  }
};

void initBLE()
{
  // Create the BLE Device
  BLEDevice::init("XIAO_ESP32S3");
  BLEDevice::setMTU(247);
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristics
  pCameraCharacteristic = pService->createCharacteristic(
      CAMERA_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);

  pAudioCharacteristic = pService->createCharacteristic(
      AUDIO_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);

  // Add descriptors
  pCameraCharacteristic->addDescriptor(new BLE2902());
  pAudioCharacteristic->addDescriptor(new BLE2902());

  // Set callbacks
  pCameraCharacteristic->setCallbacks(new MyCallbacks());
  pAudioCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE device is ready to pair");
}

// void sendImageViaBLE(camera_fb_t *fb)
// {
//   if (!deviceConnected)
//   {
//     Serial.println("No device connected");
//     return;
//   }

//   // Send image size first

//   uint32_t imageSize = fb->len;
//   pCameraCharacteristic->setValue((uint8_t *)&imageSize, sizeof(imageSize));
//   pCameraCharacteristic->notify();
//   delay(10);                // Small delay between chunks
//                             // Serial.println("Image size");
//                             // Serial.println(imageSize);
//                             // Send image data in chunks
//                             // Serial.println("Image chunks");
//   const int chunkSize = 20; // BLE packet size
//   for (int i = 0; i < fb->len; i += chunkSize)
//   {
//     int currentChunkSize = min(chunkSize, (int)(fb->len - i));
//     pCameraCharacteristic->setValue(&fb->buf[i], currentChunkSize);
//     pCameraCharacteristic->notify();
//     delay(10); // Small delay between chunks
//     // for (int j = 0; j < currentChunkSize; j++)
//     // {
//     //   byte val = fb->buf[i + j];
//     //   if (val < 0x10)
//     //     Serial.print("0");
//     //   Serial.print(val, HEX);
//     //   Serial.print(" ");
//     // }
//     // Serial.println();
//   }

//   blink();
//   // Serial.println("Image sent via BLE");
// }

void forceBLEFlush()
{
  // This forces the BLE stack to process pending operations
  esp_err_t err = esp_ble_gatts_send_indicate(
      pServer->getGattsIf(),              // GATT interface
      pServer->getConnId(),               // Connection ID
      pCameraCharacteristic->getHandle(), // Characteristic handle
      0,                                  // Data length (0 = flush)
      NULL,                               // No data
      false                               // Not indication
  );

  if (err != ESP_OK)
  {
    Serial.printf("Flush error: %d\n", err);
  }
}

void sendImageViaBLE(camera_fb_t *fb)
{
  if (!deviceConnected)
  {
    Serial.println("No device connected");
    return;
  }

  // Send image size first
  uint32_t imageSize = fb->len;
  pCameraCharacteristic->setValue((uint8_t *)&imageSize, sizeof(imageSize));
  pCameraCharacteristic->notify(); // notify() returns void, can't check return value
  delay(50);                       // Small delay between chunks

  Serial.printf("Sending image: %d bytes\n", fb->len);

  // Send image data in chunks
  const int chunkSize = 244; // BLE packet size (MTU=247, 244 bytes payload)
  int chunksSent = 0;

  for (int i = 0; i < fb->len; i += chunkSize)
  {
    if (!deviceConnected)
    {
      Serial.println("Device disconnected during transfer");
      break;
    }

    int currentChunkSize = min(chunkSize, (int)(fb->len - i));
    pCameraCharacteristic->setValue(&fb->buf[i], currentChunkSize);

    // Send notification and check if it was queued successfully
    // We can't check notify() return value directly, but we can monitor connection
    pCameraCharacteristic->notify();

    chunksSent++;

    // Add a small delay to prevent overwhelming the BLE stack
    delay(200); // Small delay between chunks

    // Print progress every 10 chunks to avoid spamming Serial
    if (chunksSent % 10 == 0)
    {
      Serial.printf("Progress: %d/%d bytes\n", min(i + chunkSize, (int)fb->len), fb->len);
    }

    // Add a slightly longer delay every 50 chunks to let BLE catch up
    if (chunksSent % 15 == 0)
    {
      forceBLEFlush();
      delay(150); // Wait for flush to complete
      Serial.printf("Forced flush after %d chunks\n", chunksSent);
    }
    else
    {
      delay(30);
    }
  }

  // Send end marker to let client know transfer is complete
  Serial.println("Sending end marker...");
  // Send a list with one element: [11111]
  uint32_t singleElementList = 11111;
  pCameraCharacteristic->setValue((uint8_t *)&singleElementList, 4);
  pCameraCharacteristic->notify();
  Serial.println("End marker notification sent");
  delay(50);

  Serial.printf("Image transfer complete. Sent %d chunks, total %d bytes\n", chunksSent, fb->len);
  blink();
}

void sendAudioViaBLE(uint8_t *data, size_t length)
{
  if (!deviceConnected)
  {
    Serial.println("No device connected");
    return;
  }

  // Send audio size first
  uint32_t audioSize = length;
  pAudioCharacteristic->setValue((uint8_t *)&audioSize, sizeof(audioSize));
  pAudioCharacteristic->notify();
  vTaskDelay(1);
  // Send audio data in chunks
  const int chunkSize = 512; // BLE packet size
  for (int i = 0; i < length; i += chunkSize)
  {
    int currentChunkSize = min(chunkSize, (int)(length - i));
    pAudioCharacteristic->setValue(&data[i], currentChunkSize);
    pAudioCharacteristic->notify();
    vTaskDelay(2); // Small delay between chunks
  }

  Serial.println("Audio sent via BLE");
}

bool isDeviceConnected()
{
  return deviceConnected;
}