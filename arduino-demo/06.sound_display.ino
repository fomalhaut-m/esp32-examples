/**
 * ESP32-S3 声音检测 + OLED 显示
 * 
 * 接线:
 *   INMP441 VDD  -> 3.3V
 *   INMP441 GND  -> GND
 *   INMP441 SCK  -> GPIO 40
 *   INMP441 WS   -> GPIO 41
 *   INMP441 SD   -> GPIO 39
 *   INMP441 L/R  -> GPIO 42
 *   OLED VCC     -> 3.3V
 *   OLED GND     -> GND
 *   OLED SCK     -> GPIO 36
 *   OLED SDA     -> GPIO 37
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include <U8g2lib.h>

#define I2S_SCK_PIN    40
#define I2S_WS_PIN     41
#define I2S_SD_PIN     39
#define I2S_LR_PIN     42

#define OLED_SCL_PIN   36
#define OLED_SDA_PIN   37

#define I2S_NUM       I2S_NUM_0
#define SAMPLE_RATE   16000
#define bufferCnt     4
#define bufferLen     128

#define HISTORY_SIZE  64

static int16_t s_buffer[bufferCnt][bufferLen];
static int volume_history[HISTORY_SIZE];
static int history_index = 0;

U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, OLED_SCL_PIN, OLED_SDA_PIN, U8X8_PIN_NONE);

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

void draw_waveform(int volume) {
    volume_history[history_index] = volume;
    history_index = (history_index + 1) % HISTORY_SIZE;
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 10, "Sound Level");
    
    char buf[8];
    sprintf(buf, "%d%%", volume);
    u8g2.drawStr(85, 10, buf);
    
    int base_y = 30;
    int height = 20;
    
    for (int x = 0; x < 128; x++) {
        int idx = (history_index - 128 + x + HISTORY_SIZE) % HISTORY_SIZE;
        if (idx < 0) idx = 0;
        int h = map(volume_history[idx], 0, 100, 0, height);
        if (h > height) h = height;
        int y = base_y - h;
        u8g2.drawPixel(x, y);
    }
    u8g2.drawHLine(0, base_y, 128);
}

void setup() {
    Serial.begin(115200);
    Serial.println("Sound Display Test");
    
    i2s_init();
    u8g2.begin();
    u8g2.clearBuffer();
    
    for (int i = 0; i < HISTORY_SIZE; i++) {
        volume_history[i] = 0;
    }
    
    Serial.println("Ready");
}

void loop() {
    size_t bytes_read = 0;
    i2s_read(I2S_NUM, &s_buffer[0], sizeof(s_buffer), &bytes_read, 100);
    
    int samples = bytes_read / sizeof(int16_t);
    if (samples > 0) {
        int32_t sum = 0;
        for (int i = 0; i < samples; i++) {
            sum += abs(s_buffer[0][i]);
        }
        int avg = sum / samples;
        int volume = map(avg, 0, 1000, 0, 100);
        if (volume > 100) volume = 100;
        
        draw_waveform(volume);
        u8g2.sendBuffer();
        
        Serial.printf("Volume: %d%%\n", volume);
    }
}