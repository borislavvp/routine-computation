#include "camera_config.h"
#include "sd_card.h"
#include "ble_transfer.h"
#include "audio_handler.h"

bool camera_sign = false;
bool audio_sign = false;
bool sd_sign = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  camera_sign = initCamera();
  audio_sign  = initAudio();
  sd_sign     = initSDCard();

  if (!camera_sign || !audio_sign || !sd_sign) {
    Serial.println("Init failed");
    while (1);
  }

  initBLE();
  Serial.println("System ready");
}

void loop() {
  /* handle BLE camera command */
  if (cameraCommandPending) {
    cameraCommandPending = false;

    camera_fb_t *fb = capturePhoto();
    if (fb) {
      char filename[32];
      time_t now = time(nullptr);
      strftime(filename, sizeof(filename),
               "/photo_%Y%m%d_%H%M%S.jpg",
               localtime(&now));
      savePhoto(filename, fb);

      startImageSend(fb);
      esp_camera_fb_return(fb);
    }
  }

  /* non-blocking BLE sender */
  processImageSend();
}
