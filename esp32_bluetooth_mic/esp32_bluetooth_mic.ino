#include "BluetoothA2DPSource.h"
#include <driver/i2s.h>

BluetoothA2DPSource a2dp_source;

// I2S pin definitions for microphone
#define I2S_WS 15
#define I2S_SD 32
#define I2S_SCK 14
#define I2S_PORT I2S_NUM_0

// Name of the Bluetooth speaker to connect to
#define BLUETOOTH_SPEAKER_NAME "My_Bluetooth_Speaker"

// Audio stream properties
#define SAMPLE_RATE 44100

// Digital amplification multiplier
#define VOLUME_MULTIPLIER 5

// Callback function to provide audio data to A2DP source
int32_t get_data_frames(Frame *data, int32_t frame_count) {
    size_t bytes_read = 0;
    
    // Read audio data from the I2S microphone
    i2s_read(I2S_PORT, data, frame_count * sizeof(Frame), &bytes_read, portMAX_DELAY);
    
    int32_t frames_read = bytes_read / sizeof(Frame);
    
    // Apply digital amplification
    for(int i = 0; i < frames_read; i++) {
        int32_t left = data[i].channel1 * VOLUME_MULTIPLIER;
        int32_t right = data[i].channel2 * VOLUME_MULTIPLIER;
        
        // Clamp values to int16_t bounds to avoid distortion/overflow
        if(left > 32767) left = 32767;
        if(left < -32768) left = -32768;
        if(right > 32767) right = 32767;
        if(right < -32768) right = -32768;
        
        data[i].channel1 = (int16_t)left;
        data[i].channel2 = (int16_t)right;
    }
    
    return frames_read;
}

void setup() {
    Serial.begin(115200);
    
    // Configure I2S interface for reading audio from mic
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };
    
    // Install and configure the I2S driver
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
    
    Serial.println("I2S microphone initialized. Starting Bluetooth A2DP...");
    
    // Start Bluetooth A2DP Source and attempt to connect to the speaker
    a2dp_source.start(BLUETOOTH_SPEAKER_NAME, get_data_frames);
}

void loop() {
    // Both Bluetooth A2DP and I2S reading run continuously in background tasks.
    // The main loop can just wait or do other minor tasks.
    delay(1000);
}
