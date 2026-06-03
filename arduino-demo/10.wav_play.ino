/**
 * ============================================================
 * ESP32-S3 WAV播放器
 * ============================================================
 * 
 * 怎么玩？
 *   按按钮1播放音乐
 *   再按一次切换下一首
 *   循环播放所有26个音效
 * 
 * 硬件接线（按规则文件）：
 *   MAX98357A DIN  → GPIO 20
 *   MAX98357A LRC  → GPIO 47
 *   MAX98357A BCLK → GPIO 21
 *   喇叭接MAX98357A的OUT+/OUT-
 *   按钮1           → GPIO 11
 * 
 * 支持的音效（26个）：
 *   exit, game_over, game_start, h1, h2
 *   key_click, num0-9, play_back, ready
 *   rec_start, rec_stop, right, wrong
 *   t1, t2, warn_humi, warn_temp
 * 
 * ============================================================
 * WAV文件说明
 * ============================================================
 * 
 * WAV文件 = 文件头(44字节) + 音频数据
 * 
 * 本示例使用的音频格式：
 *   采样率：8000 Hz
 *   位深度：8位
 *   声道：单声道
 * 
 * 原始WAV文件（32000Hz、16位）经降采样得到
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include "wav_data.h"

// ============================================================
// 引脚定义（按规则文件）
// ============================================================
#define PIN_AMP_DIN   20   // 数据输出
#define PIN_AMP_LRC   47   // 声道选择
#define PIN_AMP_BCLK  21   // 时钟
#define PIN_BTN1      11   // 按钮1

// ============================================================
// I2S配置
// ============================================================
#define I2S_PORT       I2S_NUM_1
#define SAMPLE_RATE    8000

// ============================================================
// 音效列表
// ============================================================
struct Sound {
    const char* name;
    const int8_t* data;
    uint32_t len;
};

Sound sounds[] = {
    {"exit",        WAV_exit,        WAV_exit_len},
    {"game_over",   WAV_game_over,   WAV_game_over_len},
    {"game_start",  WAV_game_start,  WAV_game_start_len},
    {"h1",          WAV_h1,          WAV_h1_len},
    {"h2",          WAV_h2,          WAV_h2_len},
    {"key_click",   WAV_key_click,   WAV_key_click_len},
    {"num0",        WAV_num0,        WAV_num0_len},
    {"num1",        WAV_num1,        WAV_num1_len},
    {"num2",        WAV_num2,        WAV_num2_len},
    {"num3",        WAV_num3,        WAV_num3_len},
    {"num4",        WAV_num4,        WAV_num4_len},
    {"num5",        WAV_num5,        WAV_num5_len},
    {"num6",        WAV_num6,        WAV_num6_len},
    {"num7",        WAV_num7,        WAV_num7_len},
    {"num8",        WAV_num8,        WAV_num8_len},
    {"num9",        WAV_num9,        WAV_num9_len},
    {"play_back",   WAV_play_back,   WAV_play_back_len},
    {"ready",       WAV_ready,       WAV_ready_len},
    {"rec_start",   WAV_rec_start,   WAV_rec_start_len},
    {"rec_stop",    WAV_rec_stop,    WAV_rec_stop_len},
    {"right",       WAV_right,       WAV_right_len},
    {"t1",          WAV_t1,          WAV_t1_len},
    {"t2",          WAV_t2,          WAV_t2_len},
    {"warn_humi",   WAV_warn_humi,   WAV_warn_humi_len},
    {"warn_temp",   WAV_warn_temp,   WAV_warn_temp_len},
    {"wrong",       WAV_wrong,       WAV_wrong_len},
};

int currentSound = 0;
int totalSounds = sizeof(sounds) / sizeof(sounds[0]);
bool isPlaying = false;

// ============================================================
// 初始化I2S
// ============================================================
void i2sInit() {
    i2s_config_t config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = 256,
        .use_apll = true,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pins = {
        .bck_io_num = PIN_AMP_BCLK,
        .ws_io_num = PIN_AMP_LRC,
        .data_out_num = PIN_AMP_DIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_PORT, &config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pins);
}

// ============================================================
// 播放8位WAV数据
// ============================================================
void playWav(const int8_t* data, uint32_t len) {
    size_t written;
    int16_t buffer[256];
    
    isPlaying = true;
    Serial.printf("Playing %u samples...\n", len);
    
    for (uint32_t i = 0; i < len; i += 256) {
        int chunk = min((uint32_t)256, len - i);
        for (int j = 0; j < chunk; j++) {
            buffer[j] = data[i + j] * 256;
        }
        i2s_write(I2S_PORT, buffer, chunk * sizeof(int16_t), &written, portMAX_DELAY);
    }
    
    isPlaying = false;
    Serial.println("Done");
}

// ============================================================
// 播放指定音效
// ============================================================
void playSound(int index) {
    if (index < 0 || index >= totalSounds) return;
    
    Serial.printf("Playing: %s\n", sounds[index].name);
    playWav(sounds[index].data, sounds[index].len);
}

// ============================================================
// 按钮检测（带消抖）
// ============================================================
bool checkButton() {
    static bool lastBtn = HIGH;
    bool btn = digitalRead(PIN_BTN1);
    
    if (btn == LOW && lastBtn == HIGH) {
        delay(50);  // 消抖
        if (digitalRead(PIN_BTN1) == LOW) {
            lastBtn = btn;
            return true;
        }
    }
    lastBtn = btn;
    return false;
}

// ============================================================
// 初始化
// ============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("WAV Player Demo");
    Serial.printf("Total sounds: %d\n", totalSounds);
    
    pinMode(PIN_BTN1, INPUT_PULLUP);
    i2sInit();
    
    Serial.println("Press BTN1 to play/next");
}

// ============================================================
// 主循环
// ============================================================
void loop() {
    if (checkButton()) {
        if (!isPlaying) {
            // 播放当前音效
            playSound(currentSound);
            // 切换到下一首
            currentSound = (currentSound + 1) % totalSounds;
        }
    }
    
    delay(10);
}
