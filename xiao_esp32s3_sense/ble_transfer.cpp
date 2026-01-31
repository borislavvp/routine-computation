#include "ble_transfer.h"
#include "audio_handler.h"
#include "camera_config.h"
#include "sd_card.h"

BLEServer *pServer = nullptr;
BLECharacteristic *pCameraCharacteristic = nullptr;
BLECharacteristic *pAudioCharacteristic = nullptr;

bool deviceConnected = false;

/* ================= LED ================= */

void blink()
{
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}

/* ================= IMAGE TX STATE ================= */

static const int IMAGE_CHUNK_SIZE = 180;

static uint8_t *imageBuf = nullptr;
static size_t imageLen = 0;
static size_t imageOffset = 0;

static bool sendingImage = false;
static bool headerSent = false;
static bool waitingAck = false;

static uint16_t imageSeq = 0;
static uint16_t totalPackets = 0;
volatile bool cameraCommandPending = false;

/* ================= BLE CALLBACKS ================= */

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer) override
  {
    deviceConnected = true;
    Serial.println("BLE connected");
    blink();
  }

  void onDisconnect(BLEServer *pServer) override
  {
    deviceConnected = false;
    Serial.println("BLE disconnected");
    BLEDevice::startAdvertising();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic) override
  {
    String value = pCharacteristic->getValue();

    if (value == "START_CAMERA")
    {
      cameraCommandPending = true;
      Serial.println("Camera command received");
    }

    if (value.startsWith("ACK:"))
    {
      uint16_t ackSeq = value.substring(4).toInt();
      Serial.printf("Received ACK for sequence %d, waiting for %d;\n", ackSeq, imageSeq);
      if (ackSeq == imageSeq || ackSeq == 0xFFFF)
      {
        Serial.printf("Packet %d ACK received, moving to %d\n", ackSeq, imageSeq);
        waitingAck = false;
      }
    }
  }
};

void initBLE()
{
  BLEDevice::init("XIAO_ESP32S3");
  BLEDevice::setMTU(247);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCameraCharacteristic = pService->createCharacteristic(
      CAMERA_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR |
          BLECharacteristic::PROPERTY_NOTIFY);

  pAudioCharacteristic = pService->createCharacteristic(
      AUDIO_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);

  pCameraCharacteristic->addDescriptor(new BLE2902());
  pAudioCharacteristic->addDescriptor(new BLE2902());

  pCameraCharacteristic->setCallbacks(new MyCallbacks());
  pAudioCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE ready");
}

/* ================= BLE CALLBACK ================= */

/* ================= IMAGE SEND ================= */

void startImageSend(camera_fb_t *fb)
{
  if (!deviceConnected || !fb)
    return;

  imageBuf = fb->buf;
  imageLen = fb->len;
  imageOffset = 0;
  imageSeq = 0;

  totalPackets = (imageLen + IMAGE_CHUNK_SIZE - 1) / IMAGE_CHUNK_SIZE;

  sendingImage = true;
  headerSent = false;
  waitingAck = false;

  Serial.printf("Begin image TX (%d bytes, %d packets)\n",
                imageLen, totalPackets);
}

void processImageSend()
{
  if (!sendingImage || !deviceConnected || waitingAck)
    return;

  /* -------- send header -------- */
  if (!headerSent)
  {
    uint8_t header[8];
    header[0] = 0xFF;
    header[1] = 0xFF;
    memcpy(header + 2, &imageLen, 4);
    memcpy(header + 6, &totalPackets, 2);

    pCameraCharacteristic->setValue(header, sizeof(header));
    pCameraCharacteristic->notify();

    headerSent = true;
    waitingAck = true;
    return;
  }

  /* -------- send data packet -------- */
  if (imageSeq < totalPackets)
  {
    
    size_t len = min((size_t)IMAGE_CHUNK_SIZE,
                     imageLen - imageOffset);
    if (imageSeq == totalPackets - 1) {
      // last packet: send all remaining bytes
      len = imageLen - imageOffset;
    }
    uint8_t packet[2 + IMAGE_CHUNK_SIZE];
    packet[0] = imageSeq >> 8;
    packet[1] = imageSeq & 0xFF;
    memcpy(packet + 2, imageBuf + imageOffset, len);

    pCameraCharacteristic->setValue(packet, len + 2);
    pCameraCharacteristic->notify();

    imageOffset += len;
    waitingAck = true;

    Serial.printf("Sent packet %d / %d\n",
                  imageSeq + 1, totalPackets);
    imageSeq++;
  }

  if (imageSeq >= totalPackets)
  {
    sendingImage = false;
    Serial.println("Image TX complete");
    delay(50);
    blink();
  }
}


void sendAudioViaBLE(uint8_t *data, size_t length)
{
  if (!deviceConnected)
    return;

  uint32_t size = length;
  pAudioCharacteristic->setValue((uint8_t *)&size, sizeof(size));
  pAudioCharacteristic->notify();
  delay(10);

  const int chunkSize = 244;
  for (size_t i = 0; i < length; i += chunkSize)
  {
    size_t len = min((size_t)chunkSize, length - i);
    pAudioCharacteristic->setValue(data + i, len);
    pAudioCharacteristic->notify();
    delay(5);
  }
}

/* ================= HELPERS ================= */

bool isDeviceConnected()
{
  return deviceConnected;
}
