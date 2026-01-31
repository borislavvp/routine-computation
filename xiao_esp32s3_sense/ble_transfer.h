#ifndef BLE_TRANSFER_H
#define BLE_TRANSFER_H

// #include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "esp_camera.h"

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CAMERA_CHARACTERISTIC_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define AUDIO_CHARACTERISTIC_UUID     "d2b5483e-36e1-4688-b7f5-ea07361b26aa"

// Function declarations
extern volatile bool cameraCommandPending;

void startImageSend(camera_fb_t *fb);
void processImageSend();

void initBLE();
void sendImageViaBLE(camera_fb_t *fb);
void sendAudioViaBLE(uint8_t* data, size_t length);
bool isDeviceConnected();

#endif 