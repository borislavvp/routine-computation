#ifndef SD_CARD_H
#define SD_CARD_H

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "esp_camera.h"

// Function declarations
bool initSDCard();
void writeFile(fs::FS &fs, const char * path, uint8_t * data, size_t len);
void listFiles(fs::FS &fs, const char * dirname);
void savePhoto(const char * fileName, camera_fb_t *fb);

#endif 