/**
 * ============================================================
 * ESP32-S3 MiniMax TTS语音合成
 * ============================================================
 * 
 * 怎么玩？
 *   按按钮1 → 调用MiniMax TTS API → 播放语音
 * 
 * 硬件接线（按规则文件）：
 *   MAX98357A DIN  → GPIO 20
 *   MAX98357A LRC  → GPIO 47
 *   MAX98357A BCLK → GPIO 21
 *   喇叭接MAX98357A的OUT+/OUT-
 *   按钮1           → GPIO 11
 * 
 * 功能说明：
 *   1. 连接WiFi
 *   2. 按按钮1调用MiniMax TTS API
 *   3. 获取返回的WAV音频（hex编码）
 *   4. 逐字符流式解码hex并播放
 * 
 * ============================================================
 * 使用前需要：
 *   1. 修改WiFi名称和密码
 *   2. 修改API Key
 *   3. 安装ESP32 Arduino库（带HttpClient）
 * ============================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <driver/i2s.h>

// ============================================================
// Hex解码
// ============================================================
static int8_t hexCharToVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// ============================================================
// WiFi配置（修改这里）
// ============================================================
const char* WIFI_SSID = "NYF_72";
const char* WIFI_PASSWORD = "woaiwojia@2019";

// ============================================================
// API配置（修改这里）
// ============================================================
const char* API_URL = "https://api.minimaxi.com/v1/t2a_v2";
const char* API_KEY = "YOUR_MINIMAX_API_KEY";

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
#define SAMPLE_RATE    32000  // MiniMax API返回的采样率

// ============================================================
// TTS配置
// ============================================================
const char* TTS_TEXT = "你好，我是智能助手，很高兴为你服务！今天天气不错，记得多喝水哦！";
const char* TTS_VOICE = "male-qn-qingse";  // MiniMax音色ID

// ============================================================
// 音频缓冲区
// ============================================================
int16_t* audioBuffer = NULL;
int audioSamples = 0;

// ============================================================
// 初始化I2S
// ============================================================
void i2sInit() {
    i2s_config_t config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
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
// 连接WiFi
// ============================================================
bool connectWiFi() {
    Serial.printf("[WiFi] Connecting to %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int timeout = 20;
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(500);
        Serial.print(".");
        timeout--;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    } else {
        Serial.println("\n[WiFi] Failed!");
        return false;
    }
}

// ============================================================
// 调用MiniMax TTS API（逐字符流式处理）
// ============================================================
bool callTTS() {
    Serial.println("[TTS] Calling MiniMax API...");
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[TTS] WiFi not connected!");
        return false;
    }
    
    HTTPClient http;
    http.begin(API_URL);
    http.setTimeout(60000);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + API_KEY);
    http.addHeader("Accept-Encoding", "identity");
    
    // 构建请求JSON
    String json = "{";
    json += "\"model\":\"speech-2.8-hd\",";
    json += "\"text\":\"" + String(TTS_TEXT) + "\",";
    json += "\"stream\":false,";
    json += "\"voice_setting\":{";
    json += "\"voice_id\":\"" + String(TTS_VOICE) + "\",";
    json += "\"speed\":1,";
    json += "\"vol\":1,";
    json += "\"pitch\":0,";
    json += "\"emotion\":\"happy\"";
    json += "},";
    json += "\"audio_setting\":{";
    json += "\"sample_rate\":32000,";
    json += "\"bitrate\":128000,";
    json += "\"format\":\"wav\",";
    json += "\"channel\":1";
    json += "},";
    json += "\"subtitle_enable\":false";
    json += "}";
    
    Serial.printf("[TTS] Request: %d bytes\n", json.length());
    
    int httpCode = http.POST(json);
    Serial.printf("[TTS] HTTP code: %d, size: %d\n", httpCode, http.getSize());
    
    if (httpCode != 200) {
        Serial.printf("[TTS] HTTP error: %d\n", httpCode);
        String err = http.getString();
        if (err.length() > 0) Serial.println(err);
        http.end();
        return false;
    }
    
    // 获取响应流，逐字符处理（不占用大内存）
    WiFiClient &stream = http.getStream();
    
    // 状态机解析JSON
    enum { S_NORMAL, S_KEY, S_VALUE } state = S_NORMAL;
    
    int sampleRate = 32000;
    int channels = 1;
    int statusCode = -1;
    
    char keyBuf[32];
    int keyLen = 0;
    char valBuf[32];
    int valLen = 0;
    char numBuf[16];
    int numLen = 0;
    
    // PCM缓冲区（870KB hex ≈ 435KB PCM）
    #define MAX_PCM_BYTES 500000
    uint8_t *pcm = NULL;
    int pcmBytes = 0;
    bool audioDecoding = false;
    bool audioFound = false;
    
    int wavSkipLeft = 88;  // 跳过WAV头（44字节 = 88个hex字符）
    bool hexHigh = true;
    uint8_t hexByte = 0;
    int totalHexRead = 0;
    
    unsigned long startMs = millis();
    
    while (stream.connected() || stream.available()) {
        if (millis() - startMs > 60000) { Serial.println("[TTS] Timeout!"); break; }
        if (!stream.available()) { delay(1); continue; }
        int c = stream.read();
        if (c < 0) break;
        
        // ===== hex解码模式：直接处理每个字符 =====
        if (audioDecoding) {
            if (c == '"') {
                // hex数据结束（遇到JSON引号）
                audioDecoding = false;
                Serial.printf("[TTS] Hex end. Total: %d chars\n", totalHexRead);
                break;
            }
            int8_t val = hexCharToVal(c);
            if (val < 0) continue;
            totalHexRead++;
            if (wavSkipLeft > 0) { wavSkipLeft--; continue; }
            if (hexHigh) {
                hexByte = val << 4;
                hexHigh = false;
            } else {
                hexByte |= val;
                hexHigh = true;
                if (pcm && pcmBytes < MAX_PCM_BYTES) {
                    pcm[pcmBytes++] = hexByte;
                }
            }
            continue;
        }
        
        // ===== JSON解析模式 =====
        if (c == '"') {
            if (state == S_NORMAL) {
                state = S_KEY; keyLen = 0;
            } else if (state == S_KEY) {
                state = S_VALUE; keyBuf[keyLen] = '\0'; valLen = 0;
            } else if (state == S_VALUE) {
                valBuf[valLen] = '\0';
                state = S_NORMAL;
            }
            continue;
        }
        
        if (state == S_KEY) {
            if (keyLen < 31) keyBuf[keyLen++] = c;
            continue;
        }
        
        if (state == S_VALUE) {
            // audio字段：开始hex解码
            if (strcmp(keyBuf, "audio") == 0 && valLen == 0) {
                audioDecoding = true;
                audioFound = true;
                pcm = (uint8_t*)malloc(MAX_PCM_BYTES);
                if (!pcm) {
                    Serial.printf("[TTS] Cannot alloc %d bytes\n", MAX_PCM_BYTES);
                    http.end(); return false;
                }
                Serial.printf("[TTS] Audio: %dHz, %dch — decoding hex...\n", sampleRate, channels);
                i2s_set_clk(I2S_PORT, sampleRate, I2S_BITS_PER_SAMPLE_16BIT, (i2s_channel_t)channels);
                // 处理当前字符
                int8_t val = hexCharToVal(c);
                if (val >= 0) {
                    totalHexRead++;
                    if (wavSkipLeft > 0) { wavSkipLeft--; }
                    else { hexByte = val << 4; hexHigh = false; }
                }
                continue;
            }
            valBuf[valLen++] = c;
            if (valLen < 31) valBuf[valLen] = '\0';
            continue;
        }
        
        // 数字值（引号外，如 "status_code": 0）
        if (state == S_NORMAL && c >= '0' && c <= '9') {
            if (numLen < 15) numBuf[numLen++] = c;
            numBuf[numLen] = '\0';
            continue;
        }
        if (state == S_NORMAL && numLen > 0 && (c == ',' || c == '}' || c == ']')) {
            numBuf[numLen] = '\0';
            // 处理数字字段
            if (strcmp(keyBuf, "status_code") == 0) {
                statusCode = atoi(numBuf);
                if (statusCode != 0) Serial.printf("[TTS] API error: %d\n", statusCode);
            } else if (strcmp(keyBuf, "audio_sample_rate") == 0) {
                sampleRate = atoi(numBuf);
            } else if (strcmp(keyBuf, "audio_channel") == 0) {
                channels = atoi(numBuf);
            }
            numLen = 0; keyLen = 0;
            continue;
        }
    }
    
    http.end();
    
    if (!audioFound || !pcm) {
        Serial.println("[TTS] No audio data received!");
        return false;
    }
    
    // 释放未用完的内存
    if (pcmBytes < MAX_PCM_BYTES) {
        pcm = (uint8_t*)realloc(pcm, pcmBytes);
    }
    
    audioBuffer = (int16_t*)pcm;
    audioSamples = pcmBytes / 2;
    Serial.printf("[TTS] Done! %d hex -> %d PCM -> %d samples\n", totalHexRead, pcmBytes, audioSamples);
    Serial.printf("[TTS] Duration: %.2f seconds\n", (float)audioSamples / (sampleRate * channels));
    
    return audioSamples > 0;
}

// ============================================================
// 播放音频
// ============================================================
void playAudio() {
    if (audioBuffer == NULL || audioSamples == 0) {
        Serial.println("[PLAY] No audio to play!");
        return;
    }
    
    Serial.printf("[PLAY] Playing %d samples...\n", audioSamples);
    
    size_t written;
    for (int i = 0; i < audioSamples; i += 256) {
        int chunk = min(256, audioSamples - i);
        i2s_write(I2S_PORT, &audioBuffer[i], chunk * sizeof(int16_t), &written, portMAX_DELAY);
    }
    
    Serial.println("[PLAY] Done!");
}

// ============================================================
// 按钮检测
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
    Serial.begin(115200);
    Serial.println("MiniMax TTS Demo");
    
    pinMode(PIN_BTN1, INPUT_PULLUP);
    i2sInit();
    
    if (!connectWiFi()) {
        Serial.println("WiFi failed! Restarting...");
        delay(3000);
        ESP.restart();
    }
    
    Serial.println("Ready! Press BTN1 to speak.");
}

// ============================================================
// 主循环
// ============================================================
void loop() {
    if (checkButton()) {
        Serial.println("=== Button pressed ===");
        
        if (callTTS()) {
            playAudio();
        }
        
        if (audioBuffer != NULL) {
            free(audioBuffer);
            audioBuffer = NULL;
        }
        
        Serial.println("=== Done ===");
    }
    
    delay(10);
}
