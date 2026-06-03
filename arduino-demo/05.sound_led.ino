/**
 * ESP32-S3 声音控制 LED
 * 
 * 接线:
 *   INMP441 VDD  -> 3.3V
 *   INMP441 GND  -> GND
 *   INMP441 SCK  -> GPIO 40
 *   INMP441 WS   -> GPIO 41
 *   INMP441 SD   -> GPIO 39
 *   INMP441 L/R  -> GPIO 42
 *   LED红 -> GPIO 15 (串联220Ω, 负极 -> GND)
 *   LED黄 -> GPIO 16 (串联220Ω, 负极 -> GND)
 *   LED绿 -> GPIO 17 (串联220Ω, 负极 -> GND)
 */

#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_SCK_PIN    40
#define I2S_WS_PIN     41
#define I2S_SD_PIN     39
#define I2S_LR_PIN     42

#define LED_RED_PIN    15
#define LED_YELLOW_PIN 16
#define LED_GREEN_PIN  17

#define THRESHOLD_SOFT   100
#define THRESHOLD_MEDIUM 300
#define THRESHOLD_LOUD   600

#define I2S_NUM       I2S_NUM_0
#define SAMPLE_RATE   16000
#define bufferCnt     4
#define bufferLen     256

static int16_t s_buffer[bufferCnt][bufferLen];

void i2s_init() {
    pinMode(I2S_LR_PIN, OUTPUT);
    digitalWrite(I2S_LR_PIN, LOW);
    
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = bufferCnt,
        .dma_buf_len = bufferLen,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD_PIN
    };

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM);
}

void setup() {
    Serial.begin(115200);
    Serial.println("Sound LED Test");
    
    i2s_init();
    
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    
    Serial.println("Ready");
}

void all_led_off() {
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_YELLOW_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, LOW);
}

void loop() {
    size_t bytes_read = 0;
    i2s_read(I2S_NUM, &s_buffer[0], sizeof(s_buffer), &bytes_read, portMAX_DELAY);
    
    int samples = bytes_read / sizeof(int16_t);
    if (samples > 0) {
        int32_t sum = 0;
        for (int i = 0; i < samples; i++) {
            sum += abs(s_buffer[0][i]);
        }
        int avg = sum / samples;
        
        all_led_off();
        
        if (avg >= THRESHOLD_LOUD) {
            digitalWrite(LED_RED_PIN, HIGH);
            Serial.printf("Loud! %d\n", avg);
        } else if (avg >= THRESHOLD_MEDIUM) {
            digitalWrite(LED_YELLOW_PIN, HIGH);
            Serial.printf("Medium %d\n", avg);
        } else if (avg >= THRESHOLD_SOFT) {
            digitalWrite(LED_GREEN_PIN, HIGH);
            Serial.printf("Soft  %d\n", avg);
        }
    }
}