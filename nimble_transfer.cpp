#include "nimble_transfer.h"
#include "audio_handler.h"
#include "camera_config.h"
#include "sd_card.h"
#include <NimBLEDevice.h>

NimBLEServer *pServer = nullptr;
NimBLECharacteristic *pCameraCharacteristic = nullptr;
NimBLECharacteristic *pAudioCharacteristic = nullptr;
bool deviceConnected = false;

// Corrected server callbacks
class MyServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) override {
        deviceConnected = true;
        Serial.println("Device connected");
    }

    void onDisconnect(NimBLEServer* pServer) override {
        deviceConnected = false;
        Serial.println("Device disconnected");
        NimBLEDevice::startAdvertising(); // restart advertising
    }
};

// Characteristic callbacks
class MyCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic) override
    {
        std::string value = pCharacteristic->getValue();

        if (value == "START_CAMERA")
        {
            camera_fb_t *fb = capturePhoto();
            if (fb)
            {
                // Save to SD card
                char filename[32];
                time_t now = time(nullptr);
                strftime(filename, sizeof(filename), "/photo_%Y%m%d_%H%M%S.jpg", localtime(&now));
                savePhoto(filename, fb);

                // Send via BLE
                sendImageViaBLE(fb);

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
            uint8_t *audioData = getAudioData();
            size_t audioSize = getAudioDataSize();
            if (audioData && audioSize > 0)
            {
                sendAudioViaBLE(audioData, audioSize);
            }
        }
    }
};

void initBLE()
{
    NimBLEDevice::init("XIAO_ESP32S3");
    NimBLEDevice::setMTU(247);

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    NimBLEService *pService = pServer->createService(SERVICE_UUID);

    pCameraCharacteristic = pService->createCharacteristic(
        CAMERA_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::NOTIFY);

    pAudioCharacteristic = pService->createCharacteristic(
        AUDIO_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::NOTIFY);

    pCameraCharacteristic->addDescriptor(new NimBLE2902());
    pAudioCharacteristic->addDescriptor(new NimBLE2902());

    pCameraCharacteristic->setCallbacks(new MyCallbacks());
    pAudioCharacteristic->setCallbacks(new MyCallbacks());

    pService->start();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    NimBLEDevice::startAdvertising();

    Serial.println("BLE device is ready to pair");
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
    pCameraCharacteristic->notify();
    vTaskDelay(2);

    const int chunkSize = 50;
    for (int i = 0; i < fb->len; i += chunkSize)
    {
        int currentChunkSize = min(chunkSize, (int)(fb->len - i));
        bool isLastChunk = (i + currentChunkSize >= fb->len);

        if (isLastChunk)
        {
            static uint8_t buffer[chunkSize + 2];
            memcpy(buffer, &fb->buf[i], currentChunkSize);
            buffer[currentChunkSize] = 255;     // 0xFF
            buffer[currentChunkSize + 1] = 217; // 0xD9

            pCameraCharacteristic->setValue(buffer, currentChunkSize + 2);
            pCameraCharacteristic->notify();
            vTaskDelay(10);
        }
        else
        {
            pCameraCharacteristic->setValue(&fb->buf[i], currentChunkSize);
            pCameraCharacteristic->notify();
            vTaskDelay(5);
        }
    }
}

void sendAudioViaBLE(uint8_t *data, size_t length)
{
    if (!deviceConnected)
    {
        Serial.println("No device connected");
        return;
    }

    uint32_t audioSize = length;
    pAudioCharacteristic->setValue((uint8_t *)&audioSize, sizeof(audioSize));
    pAudioCharacteristic->notify();
    vTaskDelay(2);

    const int chunkSize = 180;
    for (int i = 0; i < length; i += chunkSize)
    {
        int currentChunkSize = min(chunkSize, (int)(length - i));
        pAudioCharacteristic->setValue(&data[i], currentChunkSize);
        pAudioCharacteristic->notify();
        vTaskDelay(5);
    }
    Serial.println("Audio sent via BLE");
}

bool isDeviceConnected()
{
    return deviceConnected;
}
