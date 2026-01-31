#ifndef AUDIO_HANDLER_H
#define AUDIO_HANDLER_H

#include <Arduino.h>
#include <ESP_I2S.h>
#include "sd_card.h"

// I2S configuration
#define SAMPLE_RATE     16000U
#define SAMPLE_BITS     I2S_DATA_BIT_WIDTH_16BIT
#define WAV_HEADER_SIZE 44
#define RECORD_TIME     10  // seconds
#define VOLUME_GAIN     2   // Volume gain factor

// Function declarations
bool initAudio();
void startRecording();
void stopRecording();
bool isRecording();
uint8_t* getAudioData();
size_t getAudioDataSize();

#endif 