/**
 * ESP32 按钮控制 LED 实验
 * 
 * 原理说明：
 * 1. INPUT_PULLUP 模式：芯片内部有上拉电阻，按钮未按时读到 HIGH，按下时拉到 LOW
 * 2. Toggle 模式：记录上次状态，当状态从 HIGH 变 LOW 时（即按下瞬间）切换 LED
 * 3. 消抖：delay(50) 避免机械按键抖动导致的误触发
 */

// 按钮引脚定义 (GPIO 11, 12, 13)
const int buttonPins[] = {11, 12, 13};

// LED 引脚定义 (GPIO 15, 16, 17)
const int ledPins[] = {15, 16, 17};

// 按钮数量
const int BUTTON_COUNT = 3;

// 记录每个按钮的上次状态（用于检测状态变化）
// 初始值为 HIGH，因为 INPUT_PULLUP 模式下默认是 HIGH
bool lastButtonState[] = {HIGH, HIGH, HIGH};

void setup() {
    Serial.begin(115200);  // 初始化串口，波特率 115200
    
    // 初始化所有引脚
    for (int i = 0; i < BUTTON_COUNT; i++) {
        pinMode(buttonPins[i], INPUT_PULLUP);  // 按钮引脚：输入+内部上拉
        pinMode(ledPins[i], OUTPUT);             // LED 引脚：输出
    }
    
    Serial.println("System ready");
}

void loop() {
    // 遍历每个按钮
    for (int i = 0; i < BUTTON_COUNT; i++) {
        // 读取当前按钮状态
        int state = digitalRead(buttonPins[i]);
        
        /**
         * 状态变化检测：
         * lastButtonState[i] == HIGH  (上次是松开)
         * state == LOW               (这次是按下)
         * 只有满足这两个条件才是"按下瞬间"
         * 
         * 这样可以避免按住不放时一直触发
         */
        if (state == LOW && lastButtonState[i] == HIGH) {
            // Toggle: 读取当前 LED 状态并取反
            // digitalRead(ledPins[i]) 返回当前状态
            // ! 取反后写入，实现开/关切换
            digitalWrite(ledPins[i], !digitalRead(ledPins[i]));
            
            // 打印状态
            Serial.printf("Button %d -> LED %d is %s\n", 
                i + 1, 
                i + 1, 
                digitalRead(ledPins[i]) ? "ON" : "OFF");
        }
        
        // 更新上次状态，为下次检测做准备
        lastButtonState[i] = state;
    }
    
    // 延时 50ms：
    // - 避免 CPU 空转
    // - 消抖：机械按键按下时会有几 ms 的抖动
    delay(50);
}