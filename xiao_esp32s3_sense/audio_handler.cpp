#include "audio_handler.h"
#include "sd_card.h"

static bool recording = false;
static uint8_t* audioBuffer = NULL;
static size_t audioBufferSize = 0;
static TaskHandle_t recordTaskHandle = NULL;
I2SClass i2s;

void recordTask(void *parameter) {
    // Record audio using the built-in WAV recording function
    audioBuffer = i2s.recordWAV(RECORD_TIME, &audioBufferSize);
    
    if (audioBuffer == NULL) {
        Serial.println("Record Failed!");
        audioBufferSize = 0;
    } else {
        Serial.printf("Recorded %d bytes\n", audioBufferSize);
        
        // Increase volume
        for (uint32_t i = WAV_HEADER_SIZE; i < audioBufferSize; i += 2) {
            (*(uint16_t *)(audioBuffer + i)) <<= VOLUME_GAIN;
        }

        // Save to SD card with timestamp
        char filename[32];
        time_t now = time(nullptr);
        strftime(filename, sizeof(filename), "/audio_%Y%m%d_%H%M%S.wav", localtime(&now));
        
        // Create a copy of the buffer for SD card writing
        uint8_t* sdBuffer = (uint8_t*)malloc(audioBufferSize);
        if (sdBuffer != NULL) {
            memcpy(sdBuffer, audioBuffer, audioBufferSize);
            writeFile(SD, filename, sdBuffer, audioBufferSize);
            free(sdBuffer);
        } else {
            Serial.println("Failed to allocate memory for SD card writing");
        }
    }
    
    recording = false;
    vTaskDelete(NULL);
}

bool initAudio() {
    // Set I2S pins for XIAO ESP32S3 Sense
    i2s.setPinsPdmRx(42, 41);  // CLK, DIN0
    
    // Initialize I2S in PDM RX mode
    if (!i2s.begin(I2S_MODE_PDM_RX, SAMPLE_RATE, SAMPLE_BITS, I2S_SLOT_MODE_MONO)) {
        Serial.println("Failed to initialize I2S!");
        return false;
    }
    
    return true;
}

void startRecording() {
    if (!recording) {
        // Free any existing buffer
        if (audioBuffer != NULL) {
            free(audioBuffer);
            audioBuffer = NULL;
        }
        audioBufferSize = 0;
        
        recording = true;
        xTaskCreate(recordTask, "RecordTask", 4096, NULL, 1, &recordTaskHandle);
    }
}

void stopRecording() {
    recording = false;
    if (recordTaskHandle != NULL) {
        vTaskDelete(recordTaskHandle);
        recordTaskHandle = NULL;
    }
}

bool isRecording() {
    return recording;
}

uint8_t* getAudioData() {
    return audioBuffer;
}

size_t getAudioDataSize() {
    return audioBufferSize;
} 