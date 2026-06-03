/**
 * ============================================================
 * ESP32-S3 INMP441 MEMS麦克风示例
 * ============================================================
 * 
 * 学习目标：采集声音并计算音量
 * 
 * INMP441 参数：
 * - 接口：I2S数字输出
 * - 采样率：支持8k-96kHz
 * - 位深：24-bit (我们取16-bit)
 * - 灵敏度：-26dBFS
 * 
 * 接线：
 *   INMP441模块  →  ESP32-S3
 *   VDD (电源+) →  3.3V
 *   GND (电源-) →  GND
 *   SCK (时钟)  →  GPIO 40
 *   WS  (字选)  →  GPIO 41
 *   SD  (数据)  →  GPIO 39
 *   L/R (声道)  →  GPIO 42 (可接高/低电平选择声道)
 */

// ============================================================
// 第1步：加载库
// ============================================================

#include <Arduino.h>          // Arduino基础库
#include <driver/i2s.h>          // ESP32的I2S驱动库

// ============================================================
// 第2步：定义I2S引脚
// ============================================================

#define I2S_SCK_PIN   40   // SCK/BCLK：I2S时钟信号
#define I2S_WS_PIN    41   // WS/LRCK：字选择时钟
#define I2S_SD_PIN     39   // SD/DOUT：数据输出
#define I2S_LR_PIN     42   // L/R：声道选择引脚

// ============================================================
// 第3步：定义参数
// ============================================================

#define I2S_NUM       I2S_NUM_0   // 使用I2S端口0
#define SAMPLE_RATE   16000           // 采样率：每秒采集16000个样本
#define bufferCnt     4                // DMA缓冲区数量
#define bufferLen     256              // 每个缓冲区大小

// ============================================================
// 第4步：创建缓冲区
// ============================================================

// int16_t：有符号16位整数（范围 -32768 到 32767）
// 二维数组：bufferCnt个缓冲区，每个bufferLen个样本
static int16_t s_buffer[bufferCnt][bufferLen];

// ============================================================
// 第5步：I2S初始化函数
// ============================================================

void i2s_init() {
    // 设置声道选择
    // LOW  = 左声道
    // HIGH = 右声道
    pinMode(I2S_LR_PIN, OUTPUT);
    digitalWrite(I2S_LR_PIN, LOW);  // 选择左声道
    
    // I2S配置结构体
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),  // 主机+接收模式
        .sample_rate = SAMPLE_RATE,                       // 采样率
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,    // 16位采样
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,      // 只接收左声道
        .communication_format = I2S_COMM_FORMAT_STAND_I2S, // 标准I2S格式
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,         // 中断优先级
        .dma_buf_count = bufferCnt,                      // DMA缓冲区数量
        .dma_buf_len = bufferLen,                       // DMA缓冲区大小
        .use_apll = false,                             // 不使用APLL时钟
        .tx_desc_auto_clear = false,                    // 发送描述符设置
        .fixed_mclk = 0                               // 自动MCLK分频
    };

    // 引脚配置
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN,       // BCLK引脚
        .ws_io_num = I2S_WS_PIN,        // WS引脚
        .data_out_num = I2S_PIN_NO_CHANGE, // 不使用发送
        .data_in_num = I2S_SD_PIN        // 数据输入引脚
    };

    // 安装I2S驱动
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    
    // 设置引脚
    i2s_set_pin(I2S_NUM, &pin_config);
    
    // 清空DMA缓冲区
    i2s_zero_dma_buffer(I2S_NUM);
}

// ============================================================
// 第6步：初始化（程序启动时执行一次）
// ============================================================

void setup() {
    // 启动串口
    Serial.begin(115200);
    Serial.println("INMP441 Microphone Test");
    
    // 初始化I2S
    i2s_init();
    
    Serial.println("I2S Init OK");
    Serial.printf("SCK=GPIO%d, WS=GPIO%d, SD=GPIO%d\n", 
                  I2S_SCK_PIN, I2S_WS_PIN, I2S_SD_PIN);
}

// ============================================================
// 第7步：主循环
// ============================================================

void loop() {
    size_t bytes_read = 0;  // 存储读取到的字节数
    
    // 读取I2S数据
    // i2s_read(端口, 缓冲区, 缓冲区大小, 实际读取的字节数, 超时时间)
    i2s_read(I2S_NUM, &s_buffer[0], sizeof(s_buffer), &bytes_read, portMAX_DELAY);
    
    // 计算样本数量
    // sizeof(int16_t) = 2字节
    int samples = bytes_read / sizeof(int16_t);
    
    if (samples > 0) {
        // 计算音量
        int32_t sum = 0;
        
        // 遍历所有样本
        for (int i = 0; i < samples; i++) {
            // abs()：取绝对值（声音有正有负）
            sum += abs(s_buffer[0][i]);
        }
        
        // 求平均值
        int avg = sum / samples;
        
        // map()：映射函数
        // 把 0-2000 的值映射到 0-100
        int vol = map(avg, 0, 2000, 0, 100);
        if (vol > 100) vol = 100;  // 限制最大值
        
        // ==========================================
        // 打印音量条
        // ==========================================
        Serial.printf("Volume: %3d%% |", vol);
        
        // vol/5 = 进度条长度（20个#）
        for (int i = 0; i < vol/5; i++) Serial.print("#");
        for (int i = 0; i < 20-vol/5; i++) Serial.print("-");
        Serial.println("|");
    }
}

/**
 * ============================================================
 * I2S通信协议说明
 * ============================================================
 * 
 * 【I2S三根线】
 * - BCLK (SCK)：时钟线，提供位时钟
 * - WS (LRCK)：字选择时钟，区分左右声道
 * - SD (DATA)：数据线，传输音频数据
 * 
 * 【I2S时序】
 * - 主机发送BCLK时钟
 * - WS决定当前传输左声道还是右声道
 * - SD在时钟上升沿/下降沿采样数据
 * 
 * 【麦克风数据】
 * - INMP441输出24-bit，我们取高16-bit
 * - 采样率16000Hz = 每秒采集16000个样本
 */