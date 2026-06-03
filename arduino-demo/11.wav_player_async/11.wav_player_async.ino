/**
 * ============================================================
 * ESP32-S3 异步WAV播放器
 * ============================================================
 * 
 * 怎么玩？
 *   按按钮1播放音乐
 *   播放中再按按钮1 → 停止/跳到下一首
 *   播放完自动切换下一首
 * 
 * 硬件接线（按规则文件）：
 *   MAX98357A DIN  → GPIO 20
 *   MAX98357A LRC  → GPIO 47
 *   MAX98357A BCLK → GPIO 21
 *   喇叭接MAX98357A的OUT+/OUT-
 *   按钮1           → GPIO 11
 * 
 * 异步播放原理：
 *   用状态机控制播放，每次loop只发送一小块数据
 *   这样按钮可以随时响应，不会被阻塞
 * 
 * 支持的音效（26个）：
 *   exit, game_over, game_start, h1, h2
 *   key_click, num0-9, play_back, ready
 *   rec_start, rec_stop, right, wrong
 *   t1, t2, warn_humi, warn_temp
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include "wav_data.h"

// ============================================================
// 引脚定义
// ============================================================
#define PIN_AMP_DIN   20
#define PIN_AMP_LRC   47
#define PIN_AMP_BCLK  21
#define PIN_BTN1      11

// ============================================================
// I2S配置
// ============================================================
#define I2S_PORT       I2S_NUM_1
#define SAMPLE_RATE    8000

// ============================================================
// 播放状态机
// ============================================================
enum PlayState {
    STATE_IDLE,      // 空闲
    STATE_PLAYING,   // 播放中
    STATE_PAUSED     // 暂停
};

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

int totalSounds = sizeof(sounds) / sizeof(sounds[0]);

// ============================================================
// 播放器变量
// ============================================================
PlayState playState = STATE_IDLE;
int currentSound = 0;
uint32_t playIndex = 0;        // 当前播放位置
const int8_t* playData = NULL; // 当前播放数据
uint32_t playLen = 0;          // 当前播放长度
int16_t playBuffer[256];       // 播放缓冲区

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
// 开始播放（异步）
// ============================================================
void startPlay(int index) {
    if (index < 0 || index >= totalSounds) return;
    
    Serial.printf("Start: %s\n", sounds[index].name);
    
    playData = sounds[index].data;
    playLen = sounds[index].len;
    playIndex = 0;
    playState = STATE_PLAYING;
}

// ============================================================
// 停止播放
// ============================================================
void stopPlay() {
    playState = STATE_IDLE;
    playIndex = 0;
    Serial.println("Stop");
}

// ============================================================
// 播放更新（每次loop调用，只发一小块）
// ============================================================
void playUpdate() {
    if (playState != STATE_PLAYING) return;
    if (playIndex >= playLen) {
        // 播放完成，自动下一首
        Serial.println("Done");
        currentSound = (currentSound + 1) % totalSounds;
        startPlay(currentSound);
        return;
    }
    
    // 每次发送64个采样（约8ms）
    int chunk = min((uint32_t)64, playLen - playIndex);
    for (int j = 0; j < chunk; j++) {
        playBuffer[j] = playData[playIndex + j] * 256;
    }
    
    size_t written;
    i2s_write(I2S_PORT, playBuffer, chunk * sizeof(int16_t), &written, portMAX_DELAY);
    
    playIndex += chunk;
}

// ============================================================
// 按钮检测（带消抖）
// ============================================================
bool checkButton() {
    static bool lastBtn = HIGH;
    bool btn = digitalRead(PIN_BTN1);
    
    if (btn == LOW && lastBtn == HIGH) {
        delay(50);
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
    Serial.begin(11200);
    Serial.println("Async WAV Player");
    Serial.printf("Total sounds: %d\n", totalSounds);
    
    pinMode(PIN_BTN1, INPUT_PULLUP);
    i2sInit();
    
    Serial.println("Press BTN1 to play/pause/next");
}

// ============================================================
// 主循环（异步）
// ============================================================
void loop() {
    // 1. 按钮处理
    if (checkButton()) {
        switch (playState) {
            case STATE_IDLE:
                // 空闲 → 播放当前
                startPlay(currentSound);
                break;
                
            case STATE_PLAYING:
                // 播放中 → 停止，切换下一首
                stopPlay();
                currentSound = (currentSound + 1) % totalSounds;
                break;
                
            case STATE_PAUSED:
                // 暂停 → 继续播放
                playState = STATE_PLAYING;
                break;
        }
    }
    
    // 2. 异步播放更新（非阻塞）
    playUpdate();
    
    // 3. 小延时
    delay(1);
}
