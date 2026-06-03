/**
 * ============================================================
 * ESP32-S3 多功能趣味交互装置（面向对象版本）
 * ============================================================
 * 
 * 
 * ===================== 怎么玩？=====================
 * 
 * 【按钮1 - 天气播报】
 *   按一下按钮1
 *   → 绿灯亮
 *   → OLED显示当前温度和湿度
 *   → 喇叭播报温度/湿度语音
 *   → 如果温度>30℃，红灯亮 + 报警"温度过高"
 * 
 * 【按钮2 - 录音留言】
 *   按一下按钮2
 *   → 黄灯亮，OLED显示"录音中..."
 *   → 麦克风开始收音
 *   → 再按一下停止录音，自动播放刚才录的声音
 * 
 * 【按钮3 - 灯光游戏】
 *   按一下按钮3启动游戏
 *   → 红灯随机亮起
 *   → 最快按下按钮的人得分
 *   → 语音+OLED播报分数
 * 
 * 【待机状态】
 *   → 绿灯呼吸灯（一闪一闪）
 *   → OLED显示"Multi-Device Ready!"
 * 
 * 
 * ===================== 硬件接线 =====================
 * 
 *   功放模块(MAX98357A) →  GPIO 20/47/21
 *   麦克风(INMP441)     →  GPIO 39/40/41
 *   温湿度(DHT11)        →  GPIO 4
 *   屏幕(OLED)           →  GPIO 36/37
 *   红灯                  →  GPIO 15
 *   黄灯                  →  GPIO 16
 *   绿灯                  →  GPIO 17
 *   按钮1/2/3              →  GPIO 11/12/13
 * 
 * 
 * ===================== 音效列表 =====================
 * 
 *  使用wav_data.h中的26个音效：
 *   - ready: 启动音效
 *   - t1, t2: 温度提示
 *   - h1, h2: 湿度提示
 *   - warn_temp, warn_humi: 报警音
 *   - rec_start, rec_stop: 录音提示
 *   - play_back: 播放提示
 *   - game_start, game_over: 游戏音效
 *   - right, wrong: 答题音效
 *   - num0-9: 数字语音
 * 
 * 
 * ===================== 学习目标 =====================
 * 
 *  面向对象编程：将每个模块封装成类
 *   - LED类：封装LED控制
 *   - Button类：封装按钮检测
 *   - DHT11Sensor类：封装温湿度
 *   - OLEDDisplay类：封装屏幕
 *   - AudioPlayer类：封装音频播放（支持WAV）
 *   - Recorder类：封装录音
 */

// ============================================================
// 第1步：加载库
// ============================================================

#include <Arduino.h>
#include <driver/i2s.h>
#include <U8g2lib.h>
#include <DHT.h>
#include "wav_data.h"  // 音效数据

// ============================================================
// 第2步：定义引脚常量
// ============================================================

// 功放(MAX98357A)
const int PIN_AMP_DIN = 20;
const int PIN_AMP_LRC = 47;
const int PIN_AMP_BCLK = 21;

// 麦克风(INMP441)
const int PIN_MIC_SCK = 40;
const int PIN_MIC_WS = 41;
const int PIN_MIC_SD = 39;

// 温湿度(DHT11)
const int PIN_DHT = 4;

// 显示(OLED)
const int PIN_OLED_SCL = 36;
const int PIN_OLED_SDA = 37;

// LED
const int PIN_LED_RED = 15;
const int PIN_LED_YELLOW = 16;
const int PIN_LED_GREEN = 17;

// 按钮
const int PIN_BTN1 = 11;
const int PIN_BTN2 = 12;
const int PIN_BTN3 = 13;

// ============================================================
// 第3步：LED类
// ============================================================

class LED {
private:
    int pin;
    bool state;
    const char* name;

public:
    LED(int ledPin, const char* ledName) : pin(ledPin), state(false), name(ledName) {
        pinMode(pin, OUTPUT);
        off();
    }
    
    void on() {
        digitalWrite(pin, HIGH);
        state = true;
        Serial.printf("[LED] %s ON\n", name);
    }
    
    void off() {
        digitalWrite(pin, LOW);
        state = false;
        Serial.printf("[LED] %s OFF\n", name);
    }
    
    void toggle() {
        state ? off() : on();
    }
    
    bool isOn() {
        return state;
    }
};

// ============================================================
// 第4步：按钮类
// ============================================================

class Button {
private:
    int pin;
    int lastState;

public:
    Button(int btnPin) : pin(btnPin), lastState(HIGH) {
        pinMode(pin, INPUT_PULLUP);
    }
    
    void begin() {
        pinMode(pin, INPUT_PULLUP);
    }
    
    // 检测按下（瞬间触发）
    bool pressed() {
        int state = digitalRead(pin);
        if (state == LOW && lastState == HIGH) {
            lastState = state;
            return true;
        }
        lastState = state;
        return false;
    }
};

// ============================================================
// 第5步：温湿度传感器类
// ============================================================

class DHT11Sensor {
private:
    DHT* sensor;
    float temperature;
    float humidity;
    unsigned long lastRead;
    bool valid;

public:
    DHT11Sensor(int dhtPin) : temperature(0), humidity(0), valid(false) {
        sensor = new DHT(dhtPin, DHT11);
    }
    
    void begin() {
        sensor->begin();
        delay(2000);
        lastRead = 0;
        read();
    }
    
    void read() {
        if (millis() - lastRead > 2000) {
            temperature = sensor->readTemperature();
            humidity = sensor->readHumidity();
            valid = !isnan(temperature) && !isnan(humidity);
            lastRead = millis();
        }
    }
    
    float getTemperature() { return temperature; }
    float getHumidity() { return humidity; }
    bool isValid() { return valid; }
    bool isTooHot() { return temperature > 30; }
    bool isTooHumid() { return humidity > 80; }
};

// ============================================================
// 第6步：OLED显示类
// ============================================================

class OLEDDisplay {
public:
    U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C* u8g2;

    OLEDDisplay(int scl, int sda) {
        u8g2 = new U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C(
            U8G2_R0, scl, sda, U8X8_PIN_NONE
        );
    }
    
    void begin() {
        u8g2->begin();
        u8g2->setFont(u8g2_font_ncenB08_tr);
    }
    
    void print(const char* line1, const char* line2 = "", const char* line3 = "") {
        Serial.printf("[OLED] %s | %s | %s\n", line1, line2, line3);
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_ncenB08_tr);
        u8g2->drawStr(0, 10, line1);
        u8g2->drawStr(0, 20, line2);
        u8g2->drawStr(0, 30, line3);
        u8g2->sendBuffer();
    }
    
    void clear() {
        u8g2->clearBuffer();
        u8g2->sendBuffer();
    }
};

// ============================================================
// 第7步：音量变量（全局）
// ============================================================

int micVolume = 0;      // 麦克风音量 0-100
int speakerVolume = 0;  // 播放音量 0-100

// 更新播放音量
void updateSpeakerVolume(int16_t* data, int len) {
    if (len <= 0) return;
    long sum = 0;
    for (int i = 0; i < len; i++) {
        sum += abs(data[i]);
    }
    int avg = sum / len;
    speakerVolume = constrain(map(avg, 0, 2000, 0, 100), 0, 100);
}

// ============================================================
// 第8步：音频播放器类
// ============================================================

class AudioPlayer {
private:
    i2s_port_t i2sNum;
    int sampleRate;

public:
    AudioPlayer(i2s_port_t port, int rate) 
        : i2sNum(port), sampleRate(rate) {}
    
    void begin(int din, int lrc, int bclk) {
        i2s_config_t config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
            .sample_rate = sampleRate,
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
            .bck_io_num = bclk,
            .ws_io_num = lrc,
            .data_out_num = din,
            .data_in_num = I2S_PIN_NO_CHANGE
        };

        i2s_driver_install(i2sNum, &config, 0, NULL);
        i2s_set_pin(i2sNum, &pins);
    }
    
    // 播放WAV数据（8位转16位）
    void playWav(const int8_t* data, uint32_t len) {
        Serial.printf("[AUDIO] playWav: %u samples\n", len);
        size_t written;
        int16_t buffer[256];
        
        for (uint32_t i = 0; i < len; i += 256) {
            int chunk = min((uint32_t)256, len - i);
            for (int j = 0; j < chunk; j++) {
                buffer[j] = data[i + j] * 256;
            }
            updateSpeakerVolume(buffer, chunk);
            i2s_write(i2sNum, buffer, chunk * sizeof(int16_t), &written, portMAX_DELAY);
        }
        speakerVolume = 0;
        Serial.println("[AUDIO] playWav done");
    }
    
    // 播放16位数据
    void play16bit(const int16_t* data, uint32_t len) {
        Serial.printf("[AUDIO] play16bit: %u samples\n", len);
        size_t written;
        int16_t buffer[256];
        for (uint32_t i = 0; i < len; i += 256) {
            int chunk = min((uint32_t)256, len - i);
            memcpy(buffer, &data[i], chunk * sizeof(int16_t));
            updateSpeakerVolume(buffer, chunk);
            i2s_write(i2sNum, buffer, chunk * sizeof(int16_t), &written, portMAX_DELAY);
        }
        speakerVolume = 0;
        Serial.println("[AUDIO] play16bit done");
    }
    
    // 播放指定频率声音
    void playTone(int freq, int ms) {
        Serial.printf("[AUDIO] playTone: %dHz %dms\n", freq, ms);
        int samples = sampleRate * ms / 1000;
        int16_t buffer[256];
        
        for (int n = 0; n < samples; n += 256) {
            int chunk = min(256, samples - n);
            for (int i = 0; i < chunk; i++) {
                float t = (float)(n + i) / sampleRate;
                buffer[i] = (int16_t)(16000 * sin(2 * PI * freq * t));
            }
            updateSpeakerVolume(buffer, chunk);
            size_t written;
            i2s_write(i2sNum, buffer, chunk * sizeof(int16_t), &written, portMAX_DELAY);
        }
        speakerVolume = 0;
        Serial.println("[AUDIO] playTone done");
    }
    
    void beep() {
        playTone(440, 200);
    }
};

// ============================================================
// 第8步：录音机类
// ============================================================

class Recorder {
private:
    static const int BUFFER_SIZE = 8000;  // 0.5秒@16000Hz
    int16_t buffer[BUFFER_SIZE];
    int index;
    bool recording;
    i2s_port_t i2sPort;

public:
    Recorder(i2s_port_t port) : index(0), recording(false), i2sPort(port) {}
    
    void begin(int sck, int ws, int sd) {
        i2s_config_t config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
            .sample_rate = 16000,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 4,
            .dma_buf_len = 512,
            .use_apll = false,
            .tx_desc_auto_clear = false,
            .fixed_mclk = 0
        };

        i2s_pin_config_t pins = {
            .bck_io_num = sck,
            .ws_io_num = ws,
            .data_out_num = I2S_PIN_NO_CHANGE,
            .data_in_num = sd
        };

        i2s_driver_install(i2sPort, &config, 0, NULL);
        i2s_set_pin(i2sPort, &pins);
        i2s_zero_dma_buffer(i2sPort);
    }
    
    void start() {
        index = 0;
        recording = true;
        Serial.println("[REC] Start recording");
    }
    
    void stop() {
        recording = false;
        Serial.printf("[REC] Stop recording, %d samples\n", index);
    }
    
    // 录音采样（异步，每次loop调用）
    // 返回读取的采样数，用于音量计算
    int update() {
        if (!recording) return 0;
        if (index >= BUFFER_SIZE) {
            recording = false;
            Serial.println("[REC] Buffer full, auto stop");
            return 0;
        }
        
        int16_t sample[256];
        size_t bytes;
        i2s_read(i2sPort, sample, sizeof(sample), &bytes, 0);
        int count = bytes / sizeof(int16_t);
        
        // 调试：打印前几个采样值
        if (index == 0 && count > 0) {
            Serial.printf("[REC] First samples: %d %d %d %d\n", 
                sample[0], sample[1], sample[2], sample[3]);
        }
        
        for (int i = 0; i < count && index < BUFFER_SIZE; i++) {
            buffer[index++] = sample[i];
        }
        
        return count;  // 返回读取的采样数
    }
    
    // 获取录音缓冲区（用于音量计算）
    int16_t* getBuffer() { return buffer; }
    int getBufferSize() { return index; }
    
    // 播放录音（降采样16000→8000）
    void playback(AudioPlayer& player) {
        Serial.printf("[REC] Playback: %d samples\n", index);
        
        // 调试：打印录音数据范围
        int16_t minVal = 32767, maxVal = -32768;
        for (int i = 0; i < index; i++) {
            if (buffer[i] < minVal) minVal = buffer[i];
            if (buffer[i] > maxVal) maxVal = buffer[i];
        }
        Serial.printf("[REC] Data range: %d to %d\n", minVal, maxVal);
        
        // 每2个采样取1个，实现16000→8000降采样
        static int16_t downsampled[BUFFER_SIZE / 2];
        int len = 0;
        for (int i = 0; i < index; i += 2) {
            // 放大音量：乘以2
            int32_t sample = buffer[i] * 2;
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            downsampled[len++] = (int16_t)sample;
        }
        Serial.printf("[REC] Playing %d downsampled samples\n", len);
        player.play16bit(downsampled, len);
    }
    
    bool isRecording() { return recording; }
    int getLength() { return index; }
};

// ============================================================
// 第9步：创建全局对象
// ============================================================

LED ledRed(PIN_LED_RED, "RED");
LED ledYellow(PIN_LED_YELLOW, "YELLOW");
LED ledGreen(PIN_LED_GREEN, "GREEN");

Button btn1(PIN_BTN1);
Button btn2(PIN_BTN2);
Button btn3(PIN_BTN3);

DHT11Sensor dht11(PIN_DHT);
OLEDDisplay oled(PIN_OLED_SCL, PIN_OLED_SDA);
AudioPlayer player(I2S_NUM_1, 8000);
Recorder recorder(I2S_NUM_0);

// ============================================================
// 第10步：系统状态
// ============================================================

enum Mode { IDLE, WEATHER, RECORDING, PLAYING, GAME_WAIT, GAME_PLAY };
Mode currentMode = IDLE;

// 游戏变量
unsigned long gameStartTime = 0;
int gameScore = 0;
bool gameLedOn = false;

// ============================================================
// 第11步：数字播报函数
// ============================================================

// 播报数字（逐位播放）
void playNumber(int num) {
    if (num < 0 || num > 99) return;
    
    if (num == 0) {
        player.playWav(WAV_num0, WAV_num0_len);
        return;
    }
    
    // 十位
    int tens = num / 10;
    if (tens > 0) {
        switch (tens) {
            case 1: player.playWav(WAV_num1, WAV_num1_len); break;
            case 2: player.playWav(WAV_num2, WAV_num2_len); break;
            case 3: player.playWav(WAV_num3, WAV_num3_len); break;
            case 4: player.playWav(WAV_num4, WAV_num4_len); break;
            case 5: player.playWav(WAV_num5, WAV_num5_len); break;
            case 6: player.playWav(WAV_num6, WAV_num6_len); break;
            case 7: player.playWav(WAV_num7, WAV_num7_len); break;
            case 8: player.playWav(WAV_num8, WAV_num8_len); break;
            case 9: player.playWav(WAV_num9, WAV_num9_len); break;
        }
        delay(200);
    }
    
    // 个位
    int ones = num % 10;
    if (ones > 0 || tens == 0) {
        switch (ones) {
            case 0: player.playWav(WAV_num0, WAV_num0_len); break;
            case 1: player.playWav(WAV_num1, WAV_num1_len); break;
            case 2: player.playWav(WAV_num2, WAV_num2_len); break;
            case 3: player.playWav(WAV_num3, WAV_num3_len); break;
            case 4: player.playWav(WAV_num4, WAV_num4_len); break;
            case 5: player.playWav(WAV_num5, WAV_num5_len); break;
            case 6: player.playWav(WAV_num6, WAV_num6_len); break;
            case 7: player.playWav(WAV_num7, WAV_num7_len); break;
            case 8: player.playWav(WAV_num8, WAV_num8_len); break;
            case 9: player.playWav(WAV_num9, WAV_num9_len); break;
        }
    }
}

// ============================================================
// 第13步：音量显示
// ============================================================

// 在OLED上绘制音量条
void drawVolumeBars() {
    oled.u8g2->clearBuffer();
    oled.u8g2->setFont(u8g2_font_ncenB08_tr);
    
    // 标题
    oled.u8g2->drawStr(0, 8, "MIC");
    oled.u8g2->drawStr(64, 8, "SPK");
    
    // 麦克风音量条
    int micBar = map(micVolume, 0, 100, 0, 120);
    oled.u8g2->drawFrame(0, 12, 124, 10);
    oled.u8g2->drawBox(2, 14, micBar, 6);
    
    // 播放音量条
    int spkBar = map(speakerVolume, 0, 100, 0, 120);
    oled.u8g2->drawFrame(0, 24, 124, 10);
    oled.u8g2->drawBox(2, 26, spkBar, 6);
    
    oled.u8g2->sendBuffer();
}

// 更新麦克风音量
void updateMicVolume(int16_t* data, int len) {
    if (len <= 0) return;
    long sum = 0;
    for (int i = 0; i < len; i++) {
        sum += abs(data[i]);
    }
    micVolume = constrain(map(sum / len, 0, 2000, 0, 100), 0, 100);
}

// ============================================================
// 第15步：初始化
// ============================================================

void setup() {
    Serial.begin(115200);
    Serial.println("Multi-Device Demo");

    dht11.begin();
    oled.begin();
    player.begin(PIN_AMP_DIN, PIN_AMP_LRC, PIN_AMP_BCLK);
    recorder.begin(PIN_MIC_SCK, PIN_MIC_WS, PIN_MIC_SD);

    btn1.begin();
    btn2.begin();
    btn3.begin();

    oled.print("Multi-Device", "Ready!", "Press BTN1-3");
    player.playWav(WAV_ready, WAV_ready_len);
}

// ============================================================
// 第16步：游戏逻辑
// ============================================================

void gameStart() {
    Serial.println("[GAME] Start");
    currentMode = GAME_WAIT;
    gameScore = 0;
    oled.print("Game Start!", "Get Ready...", "");
    player.playWav(WAV_game_start, WAV_game_start_len);
    delay(1000);
    
    // 随机延迟后亮灯
    int delayMs = random(1000, 3000);
    Serial.printf("[GAME] Wait %dms\n", delayMs);
    oled.print("Wait...", "", "");
    delay(delayMs);
    
    // 亮红灯
    Serial.println("[GAME] LED ON - GO!");
    ledRed.on();
    gameLedOn = true;
    gameStartTime = millis();
    currentMode = GAME_PLAY;
    oled.print("GO! Press BTN3", "", "");
}

void gameCheck() {
    if (currentMode != GAME_PLAY) return;
    
    if (btn3.pressed() && gameLedOn) {
        // 计算反应时间
        unsigned long reaction = millis() - gameStartTime;
        gameScore++;
        Serial.printf("[GAME] Correct! Reaction: %lums Score: %d\n", reaction, gameScore);
        
        ledRed.off();
        gameLedOn = false;
        
        // 显示分数
        char buf[32];
        sprintf(buf, "Score: %d", gameScore);
        oled.print("Correct!", buf, "");
        
        // 播放正确音效
        player.playWav(WAV_right, WAV_right_len);
        delay(500);
        
        // 下一轮
        gameStart();
    }
    
    // 超时检测（3秒）
    if (millis() - gameStartTime > 3000) {
        Serial.println("[GAME] Timeout - Game Over");
        ledRed.off();
        gameLedOn = false;
        
        // 游戏结束
        char buf[32];
        sprintf(buf, "Final: %d", gameScore);
        oled.print("Time Up!", buf, "Press BTN3");
        
        player.playWav(WAV_game_over, WAV_game_over_len);
        currentMode = IDLE;
    }
}

// ============================================================
// 第17步：主循环
// ============================================================

void loop() {
    // 读取温湿度
    dht11.read();
    
    // 调试：检测按钮状态
    static int lastBtn2 = HIGH;
    int btn2State = digitalRead(PIN_BTN2);
    if (btn2State != lastBtn2) {
        Serial.printf("[DEBUG] BTN2 changed: %d -> %d\n", lastBtn2, btn2State);
        lastBtn2 = btn2State;
    }

    // 按钮1：温湿度播报
    if (btn1.pressed() && currentMode == IDLE) {
        Serial.println("[BTN1] Weather mode");
        currentMode = WEATHER;
        ledGreen.on();
        
        if (dht11.isValid()) {
            int temp = (int)dht11.getTemperature();
            int hum = (int)dht11.getHumidity();
            
            // OLED显示
            char buf[32];
            sprintf(buf, "T:%dC H:%d%%", temp, hum);
            oled.print("Weather", buf, dht11.isTooHot() ? "HOT!" : "OK");
            
            // 播报温度："温度" + 数字
            player.playWav(WAV_t1, WAV_t1_len);
            delay(300);
            playNumber(temp);
            
            // 播报湿度："湿度" + 数字
            delay(300);
            player.playWav(WAV_h1, WAV_h1_len);
            delay(300);
            playNumber(hum);
            
            // 过热报警
            if (dht11.isTooHot()) {
                delay(300);
                ledRed.on();
                player.playWav(WAV_warn_temp, WAV_warn_temp_len);
                ledRed.off();
            }
            
            // 过湿报警
            if (dht11.isTooHumid()) {
                delay(300);
                ledRed.on();
                player.playWav(WAV_warn_humi, WAV_warn_humi_len);
                ledRed.off();
            }
        } else {
            oled.print("DHT11 Error", "Check wiring", "");
            player.playTone(200, 500);
        }
        
        ledGreen.off();
        currentMode = IDLE;
        oled.print("Multi-Device", "Ready!", "Press BTN1-3");
    }

    // 按钮2：录音/播放
    if (btn2.pressed() && currentMode == IDLE) {
        Serial.println("[BTN2] Start recording");
        recorder.start();
        currentMode = RECORDING;
        ledYellow.on();
        oled.print("Recording...", "Press BTN2", "to stop");
        player.playWav(WAV_rec_start, WAV_rec_start_len);
    }
    
    if (btn2.pressed() && currentMode == RECORDING) {
        Serial.println("[BTN2] Stop recording");
        recorder.stop();
        currentMode = PLAYING;
        ledYellow.off();
        oled.print("Recording", "Stopped");
        Serial.println("[BTN2] Playing rec_stop");
        player.playWav(WAV_rec_stop, WAV_rec_stop_len);
        delay(500);
        
        Serial.printf("[BTN2] Recorded %d samples\n", recorder.getLength());
        oled.print("Playing...", "Wait", "");
        player.playWav(WAV_play_back, WAV_play_back_len);
        Serial.println("[BTN2] Playing recording...");
        recorder.playback(player);
        Serial.println("[BTN2] Playback done");
        
        currentMode = IDLE;
        oled.print("Multi-Device", "Ready!", "Press BTN1-3");
    }
    
    // 调试：打印按钮状态
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 1000) {
        lastPrint = millis();
        Serial.printf("Mode=%d BTN2=%d Recording=%d\n", 
            currentMode, digitalRead(PIN_BTN2), recorder.isRecording());
    }

    // 按钮3：游戏
    if (btn3.pressed() && currentMode == IDLE) {
        Serial.println("[BTN3] Start game");
        gameStart();
    }
    
    // 游戏检测
    gameCheck();

    // 录音采样
    if (recorder.isRecording()) {
        int samples = recorder.update();
        if (samples > 0) {
            // 更新麦克风音量（实时）
            updateMicVolume(recorder.getBuffer() + recorder.getBufferSize() - samples, samples);
        }
    }

    // 待机呼吸灯
    if (currentMode == IDLE) {
        static bool ledState = false;
        static unsigned long t = 0;
        if (millis() - t > 500) {
            t = millis();
            ledState = !ledState;
            ledState ? ledGreen.on() : ledGreen.off();
        }
        
        // 显示音量条
        drawVolumeBars();
    }
}
