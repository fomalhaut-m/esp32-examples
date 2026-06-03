/**
 * ESP32-S3 MAX98357A 功放测试
 * 
 * 接线:
 *   MAX98357A DIN  -> GPIO 20
 *   MAX98357A LRC  -> GPIO 47
 *   MAX98357A BCLK -> GPIO 21
 *   MAX98357A VIN  -> 5V (或3.3V)
 *   MAX98357A GND  -> GND
 *   MAX98357A OUT+ -> 喇叭正极
 *   MAX98357A OUT- -> 喇叭负极
 */

#include <driver/i2s.h>
#include <Arduino.h>

// 软件I2S引脚 (非标准I2S引脚)
#define I2S_NUM       I2S_NUM_1
#define I2S_BCLK_PIN  21
#define I2S_WS_PIN    47
#define I2S_DOUT_PIN  20

#define SAMPLE_RATE   44100

// 440Hz正弦波表
const int16_t sine_wave[32] = {
    0, 5271, 9616, 11791, 12199, 9616, 5271, 0,
    -5271, -9616, -11791, -12199, -11791, -9616, -5271, 0,
    0, 5271, 9616, 11791, 12199, 9616, 5271, 0,
    -5271, -9616, -11791, -12199, -11791, -9616, -5271, 0
};

void setup() {
    Serial.begin(115200);
    Serial.println("MAX98357A Test");
    Serial.printf("DIN=GPIO%d, LRC=GPIO%d, BCLK=GPIO%d\n", I2S_DOUT_PIN, I2S_WS_PIN, I2S_BCLK_PIN);
    
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = 8,
        .use_apll = true,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_DOUT_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("I2S install failed: %d\n", err);
        return;
    }
    
    err = i2s_set_pin(I2S_NUM, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("I2S pin failed: %d\n", err);
        return;
    }
    
    Serial.println("I2S OK! Playing 440Hz...");
}

void loop() {
    static int idx = 0;
    
    int16_t samples[32];
    for (int i = 0; i < 32; i++) {
        samples[i] = sine_wave[(idx + i) % 32] / 4;
    }
    idx = (idx + 4) % 32;
    
    size_t written;
    i2s_write(I2S_NUM, samples, sizeof(samples), &written, portMAX_DELAY);
}