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
 *   按一下按钮3
 *   → 红灯闪烁
 *   → 玩反应力抢答游戏
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

/**
 * LED类 - 控制单个LED灯
 * 
 * 【成员变量】
 * - pin：LED连接的引脚号
 * 
 * 【成员函数】
 * - on()：开灯
 * - off()：关灯
 * - toggle()：翻转状态
 * - setBrightness()：调亮度（PWM）
 */
class LED {
private:
    int pin;         // LED引脚号
    int brightness;   // 亮度0-255

public:
    // 构造函数
    LED(int ledPin) {
        pin = ledPin;
        brightness = 0;
        pinMode(pin, OUTPUT);
        off();  // 初始关闭
    }
    
    // 开灯
    void on() {
        digitalWrite(pin, HIGH);
        brightness = 255;
    }
    
    // 关灯
    void off() {
        digitalWrite(pin, LOW);
        brightness = 0;
    }
    
    // 切换状态
    void toggle() {
        if (brightness > 0) {
            off();
        } else {
            on();
        }
    }
    
    // 调亮度(PWM)
    void setBrightness(int value) {
        brightness = constrain(value, 0, 255);
        analogWrite(pin, brightness);
    }
    
    // 获取状态
    bool isOn() {
        return brightness > 0;
    }
};

// ============================================================
// 第4步：按钮类
// ============================================================

/**
 * Button类 - 检测按钮状态，支持按下/松开/长按
 * 
 * 【成员变量】
 * - pin：按钮引脚
 * - lastState：上次状态
 * - pressTime：按下时刻
 * 
 * 【成员函数】
 * - isPressed()：是否按下
 * - isClicked()：单击事件（按下后松开）
 * - isLongPress()：长按事件
 */
class Button {
private:
    int pin;         // 按钮引脚
    int lastState;    // 上次状态
    unsigned long pressTime;  // 按下时刻
    bool pressing;     // 是否按下中

public:
    // 构造函数
    Button(int btnPin) {
        pin = btnPin;
        lastState = HIGH;
        pressing = false;
        pinMode(pin, INPUT_PULLUP);
    }
    
    // 初始化
    void begin() {
        // 设置引脚模式
        pinMode(pin, INPUT_PULLUP);
    }
    
    // 检测按下（瞬间触发）
    bool pressed() {
        int state = digitalRead(pin);
        if (state == LOW && lastState == HIGH) {
            lastState = state;
            return true;  // 下降沿触发
        }
        lastState = state;
        return false;
    }
    
    // 检测松开（瞬间触发）
    bool released() {
        int state = digitalRead(pin);
        if (state == HIGH && lastState == LOW) {
            lastState = state;
            return true;  // 上升沿触发
        }
        lastState = state;
        return false;
    }
    
    // 是否按住
    bool isPressing() {
        return digitalRead(pin) == LOW;
    }
};

// ============================================================
// 第5步：温湿度传感器类
// ============================================================

/**
 * DHT11Sensor类 - 读取温湿度
 * 
 * 【成员变量】
 * - pin：数据引脚
 * - sensor：DHT对象
 * - temp/humidity：缓存值
 * 
 * 【成员函数】
 * - begin()：初始化
 * - read()：读取数据
 * - getTemperature()：获取温度
 * - getHumidity()：获取湿度
 * - isAvailable()：数据是否有效
 */
class DHT11Sensor {
private:
    int pin;
    DHT* sensor;
    float temperature;
    float humidity;
    unsigned long lastRead;
    bool valid;

public:
    // 构造函数
    DHT11Sensor(int dhtPin) : pin(dhtPin), temperature(0), humidity(0), valid(false) {
        sensor = new DHT(dhtPin, DHT11);
    }
    
    // 初始化
    void begin() {
        sensor->begin();
        delay(2000);  // DHT11上电后需要2秒稳定
        lastRead = 0;  // 设为0，确保第一次立即读取
        read();        // 首次读取
    }
    
    // 读取数据
    void read() {
        // DHT11至少2秒读一次
        if (millis() - lastRead > 2000) {
            temperature = sensor->readTemperature();
            humidity = sensor->readHumidity();
            valid = !isnan(temperature) && !isnan(humidity);
            lastRead = millis();
        }
    }
    
    // 获取温度
    float getTemperature() {
        return temperature;
    }
    
    // 获取湿度
    float getHumidity() {
        return humidity;
    }
    
    // 数据有效？
    bool isValid() {
        return valid;
    }
    
    // 是否过热
    bool isTooHot() {
        return temperature > 30;
    }
};

// ============================================================
// 第6步：OLED显示类
// ============================================================

/**
 * OLEDDisplay类 - 控制OLED屏幕显示
 * 
 * 【成员变量】
 * - u8g2：U8g2对象
 * 
 * 【成员函数】
 * - begin()：初始化
 * - print()：显示三行文字
 * - clear()：清屏
 */
class OLEDDisplay {
private:
    U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C* u8g2;

public:
    // 构造函数
    OLEDDisplay(int scl, int sda) {
        // 创建屏幕对象
        u8g2 = new U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C(
            U8G2_R0, scl, sda, U8X8_PIN_NONE
        );
    }
    
    // 初始化
    void begin() {
        u8g2->begin();
        u8g2->setFont(u8g2_font_ncenB08_tr);
    }
    
    // 显示三行文字
    void print(const char* line1, const char* line2 = "", const char* line3 = "") {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_ncenB08_tr);
        u8g2->drawStr(0, 10, line1);
        u8g2->drawStr(0, 20, line2);
        u8g2->drawStr(0, 30, line3);
        u8g2->sendBuffer();
    }
    
    // 清屏
    void clear() {
        u8g2->clearBuffer();
        u8g2->sendBuffer();
    }
};

// ============================================================
// 第7步：音频播放器类
// ============================================================

/**
 * AudioPlayer类 - 播放音频
 * 
 * 支持两种播放方式：
 * 1. playTone() - 播放指定频率的蜂鸣声
 * 2. playWav() - 播放WAV文件数据
 */
class AudioPlayer {
private:
    i2s_port_t i2sNum;
    int sampleRate;

public:
    // 构造函数
    AudioPlayer(i2s_port_t port, int rate) 
        : i2sNum(port), sampleRate(rate) {}
    
    // 初始化
    void begin(int din, int lrc, int bclk) {
        // 配置I2S
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
        size_t written;
        int16_t buffer[256];
        
        for (uint32_t i = 0; i < len; i += 256) {
            int chunk = min((uint32_t)256, len - i);
            for (int j = 0; j < chunk; j++) {
                buffer[j] = data[i + j] * 256;  // 8位→16位
            }
            i2s_write(i2sNum, buffer, chunk * sizeof(int16_t), &written, portMAX_DELAY);
        }
    }
    
    // 播放指定频率声音
    void playTone(int freq, int ms) {
        int samples = sampleRate * ms / 1000;
        int16_t buffer[256];
        
        for (int n = 0; n < samples; n += 256) {
            int chunk = min(256, samples - n);
            for (int i = 0; i < chunk; i++) {
                float t = (float)(n + i) / sampleRate;
                buffer[i] = (int16_t)(16000 * sin(2 * PI * freq * t));
            }
            size_t written;
            i2s_write(i2sNum, buffer, chunk * sizeof(int16_t), &written, portMAX_DELAY);
        }
    }
    
    // 蜂鸣声
    void beep() {
        playTone(440, 200);
    }
};

// ============================================================
// 第8步：录音机类
// ============================================================

/**
 * Recorder类 - 录音功能
 * 
 * 【成员变量】
 * - buffer：录音缓冲区
 * - index：写入位置
 * - recording：是否录音中
 * 
 * 【成员函数】
 * - begin()：初始化
 * - start()：开始录音
 * - stop()：停止录音
 * - record()：录音采样
 * - playback()：播放录音
 */
class Recorder {
private:
    int16_t buffer[32000];
    int index;
    bool recording;
    i2s_port_t i2sPort;

public:
    Recorder(i2s_port_t port) : index(0), recording(false), i2sPort(port) {}
    
    // 初始化
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
    
    // 开始录音
    void start() {
        index = 0;
        recording = true;
    }
    
    // 停止录音
    void stop() {
        recording = false;
    }
    
    // 录音采样
    void update() {
        if (!recording) return;
        if (index >= 32000) {
            recording = false;
            return;
        }
        
        int16_t sample[256];
        size_t bytes;
        i2s_read(i2sPort, sample, sizeof(sample), &bytes, 0);
        int count = bytes / sizeof(int16_t);
        for (int i = 0; i < count && index < 32000; i++) {
            buffer[index++] = sample[i];
        }
    }
    
    // 播放录音
    void playback(AudioPlayer& player) {
        // 将16位录音数据转成16位播放（已经是16位，直接播放）
        size_t written;
        int16_t chunk[256];
        
        for (int i = 0; i < index; i += 256) {
            int len = min(256, index - i);
            memcpy(chunk, &buffer[i], len * sizeof(int16_t));
            i2s_write(i2sPort, chunk, len * sizeof(int16_t), &written, portMAX_DELAY);
        }
    }
    
    bool isRecording() {
        return recording;
    }
};

// ============================================================
// 第9步：创建全局对象
// ============================================================

LED ledRed(PIN_LED_RED);
LED ledYellow(PIN_LED_YELLOW);
LED ledGreen(PIN_LED_GREEN);

Button btn1(PIN_BTN1);
Button btn2(PIN_BTN2);
Button btn3(PIN_BTN3);

DHT11Sensor dht11(PIN_DHT);
OLEDDisplay oled(PIN_OLED_SCL, PIN_OLED_SDA);
AudioPlayer player(I2S_NUM_1, 8000);  // 8000Hz匹配WAV采样率
Recorder recorder(I2S_NUM_0);

// ============================================================
// 第10步：系统状态
// ============================================================

enum Mode { IDLE, WEATHER, RECORDING, PLAYING, GAME };
Mode currentMode = IDLE;

// ============================================================
// 第11步：初始化
// ============================================================

void setup() {
    Serial.begin(115200);
    Serial.println("Multi-Device Demo");

    // 初始化所有对象
    dht11.begin();
    oled.begin();
    player.begin(PIN_AMP_DIN, PIN_AMP_LRC, PIN_AMP_BCLK);
    recorder.begin(PIN_MIC_SCK, PIN_MIC_WS, PIN_MIC_SD);

    btn1.begin();
    btn2.begin();
    btn3.begin();

    oled.print("Multi-Device", "Ready!", "Press BTN1-3");

    player.playWav(WAV_ready, WAV_ready_len);  // 播放"准备好了"提示音
}

// ============================================================
// 第12步：主循环
// ============================================================

void loop() {
    // 读取温湿度
    dht11.read();

    // 按钮1：温湿度播报
    if (btn1.pressed()) {
        currentMode = WEATHER;
        ledGreen.on();
        
        if (dht11.isValid()) {
            char buf[32];
            sprintf(buf, "Temp: %.1fC", dht11.getTemperature());
            oled.print("Weather", buf, dht11.isTooHot() ? "HOT!" : "OK");
            
            // 播放温度提示音
            player.playWav(WAV_t1, WAV_t1_len);
            delay(200);
            
            // 播放湿度提示音
            player.playWav(WAV_h1, WAV_h1_len);
            delay(200);
            
            // 如果过热，播放报警音
            if (dht11.isTooHot()) {
                ledRed.on();
                player.playWav(WAV_warn_temp, WAV_warn_temp_len);
                ledRed.off();
            }
            
            // 如果过湿，播放报警音
            if (dht11.getHumidity() > 80) {
                ledRed.on();
                player.playWav(WAV_warn_humi, WAV_warn_humi_len);
                ledRed.off();
            }
        } else {
            oled.print("DHT11 Error", "Check wiring", "");
            // 错误提示音
            player.playTone(200, 500);
        }
    }

    // 按钮2：录音/播放
    if (btn2.pressed()) {
        if (!recorder.isRecording()) {
            recorder.start();
            currentMode = RECORDING;
            ledYellow.on();
            oled.print("Recording...", "Press BTN2", "to stop");
            player.playWav(WAV_rec_start, WAV_rec_start_len);  // 录音开始提示音
        } else {
            recorder.stop();
            currentMode = IDLE;
            ledYellow.off();
            oled.print("Recording", "Stopped");
            player.playWav(WAV_rec_stop, WAV_rec_stop_len);  // 录音结束提示音
            delay(500);
            oled.print("Playing...", "Wait", "");
            player.playWav(WAV_play_back, WAV_play_back_len);  // 播放提示音
        }
    }

    // 按钮3：游戏
    if (btn3.pressed()) {
        currentMode = GAME;
        oled.print("Reaction", "Game", "Press BTN3");
        player.playWav(WAV_game_start, WAV_game_start_len);  // 游戏开始音效
    }

    // 录音采样
    if (recorder.isRecording()) {
        recorder.update();
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
    }
}