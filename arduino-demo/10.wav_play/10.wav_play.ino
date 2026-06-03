/**
 * ============================================================
 * ESP32-S3 播放WAV文件示例
 * ============================================================
 * 
 * 功能：播放存储在Flash中的WAV音频
 * 
 * 怎么玩？
 *   上传后自动播放内置的"叮咚"提示音
 * 
 * 硬件接线（按规则文件）：
 *   MAX98357A DIN  → GPIO 20
 *   MAX98357A LRC  → GPIO 47
 *   MAX98357A BCLK → GPIO 21
 *   喇叭接MAX98357A的OUT+/OUT-
 * 
 * ============================================================
 * WAV文件格式说明
 * ============================================================
 * 
 * WAV文件由两部分组成：
 * 
 * 【文件头】44字节
 *   前4字节：'RIFF' 标识
 *   第5-8字节：文件大小
 *   第9-12字节：'WAVE' 标识
 *   第13-16字节：'fmt ' 标识
 *   第17-20字节：格式块大小(16)
 *   第21-22字节：音频格式(1=PCM)
 *   第23-24字节：声道数(1=单声道, 2=立体声)
 *   第25-28字节：采样率(如44100)
 *   第29-32字节：字节率
 *   第33-34字节：块对齐
 *   第35-36字节：位深度(8/16/32)
 *   第37-40字节：'data' 标识
 *   第41-44字节：数据大小
 * 
 * 【数据区】
 *   从第45字节开始，是实际的音频采样数据
 * 
 * ============================================================
 * 本示例的WAV数据
 * ============================================================
 * 
 * 本示例内置了一个简单的"叮咚"提示音：
 * - 采样率：16000 Hz
 * - 位深度：16位
 * - 声道：单声道
 * - 时长：约1秒
 * 
 * 如果你有自己的WAV文件，可以用以下工具转换为C数组：
 * - Audacity：导出为RAW格式
 * - 在线工具：https://notisrac.github.io/WavToC/
 * - Python脚本：用wave库读取后转成C数组
 */

#include <Arduino.h>
#include <driver/i2s.h>

// ============================================================
// 引脚定义（按规则文件）
// ============================================================
#define PIN_AMP_DIN   20   // 数据输出
#define PIN_AMP_LRC   47   // 声道选择
#define PIN_AMP_BCLK  21   // 时钟

// ============================================================
// I2S配置
// ============================================================
#define I2S_PORT       I2S_NUM_1
#define SAMPLE_RATE    16000
#define BITS_PER_SAMPLE 16

// ============================================================
// WAV文件数据（存储在Flash中）
// ============================================================
// 
// 这是一个16位、16000Hz、单声道的"叮咚"提示音
// 
// 生成方法（Python）：
// import numpy as np
// import wave
// 
// sr = 16000
// t = np.linspace(0, 1, sr)
// # 叮（高音）+ 咚（低音）
// audio = np.concatenate([
//     0.5 * np.sin(2 * np.pi * 880 * t[:sr//2]),  # 880Hz, 0.5秒
//     0.5 * np.sin(2 * np.pi * 440 * t[sr//2:]),   # 440Hz, 0.5秒
// ])
// audio = (audio * 32767).astype(np.int16)
// 
// with wave.open('dingdong.wav', 'w') as wf:
//     wf.setnchannels(1)
//     wf.setsampwidth(2)
//     wf.setframerate(sr)
//     wf.writeframes(audio.tobytes())

const int16_t wav_data[] PROGMEM = {
    // 这里用代码生成一个简单的提示音
    // 实际使用时，用上面的Python脚本生成真正的WAV数据
};

// 用代码生成提示音数据
#define WAV_SIZE 16000  // 1秒数据
int16_t wav_buffer[WAV_SIZE];

// ============================================================
// 生成提示音（叮咚声）
// ============================================================
void generateTone() {
    for (int i = 0; i < WAV_SIZE; i++) {
        float t = (float)i / SAMPLE_RATE;
        
        if (t < 0.5) {
            // 前0.5秒：880Hz（叮）
            wav_buffer[i] = (int16_t)(16000 * sin(2 * PI * 880 * t));
        } else {
            // 后0.5秒：440Hz（咚）
            wav_buffer[i] = (int16_t)(16000 * sin(2 * PI * 440 * t));
        }
    }
}

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
// 播放WAV数据
// ============================================================
void playWav(const int16_t* data, int samples) {
    size_t written;
    int offset = 0;
    
    Serial.printf("Playing %d samples...\n", samples);
    
    while (offset < samples) {
        // 每次发送256个采样
        int chunk = min(256, samples - offset);
        i2s_write(I2S_PORT, &data[offset], chunk * sizeof(int16_t), &written, portMAX_DELAY);
        offset += chunk;
    }
    
    Serial.println("Play done");
}

// ============================================================
// 播放不同音调
// ============================================================
void playBeep(int freq, int durationMs) {
    int samples = SAMPLE_RATE * durationMs / 1000;
    int16_t* buffer = (int16_t*)malloc(samples * sizeof(int16_t));
    
    if (buffer == NULL) {
        Serial.println("Memory error");
        return;
    }
    
    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        buffer[i] = (int16_t)(16000 * sin(2 * PI * freq * t));
    }
    
    playWav(buffer, samples);
    free(buffer);
}

// ============================================================
// 初始化
// ============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("WAV Player Demo");
    
    // 生成提示音
    generateTone();
    
    // 初始化I2S
    i2sInit();
    
    Serial.println("Playing ding-dong...");
    
    // 播放叮咚声
    playWav(wav_buffer, WAV_SIZE);
    
    Serial.println("Done! Entering beep mode...");
}

// ============================================================
// 主循环：按键播放不同音调
// ============================================================
void loop() {
    // 按住按钮播放不同音调
    // GPIO 0 = Boot按钮（大多数开发板都有）
    if (digitalRead(0) == LOW) {
        playBeep(440, 200);  // 440Hz
        delay(100);
    }
}
