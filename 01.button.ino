int button1Pin = 11; // 11 是按钮1的引脚
int button2Pin = 12; // 12 是按钮2的引脚
int button3Pin = 13; // 13 是按钮3的引脚


void setup() {
    Serial.begin(115200); // 初始化串口，波特率为115200
    pinMode(button1Pin, INPUT_PULLUP); // 设置按钮1引脚为输入模式，启用上拉电阻
    pinMode(button2Pin, INPUT_PULLUP); // 设置按钮2引脚为输入模式，启用上拉电阻
    pinMode(button3Pin, INPUT_PULLUP);  // 设置按钮3引脚为输入模式，启用上拉电阻

    Serial.println("button setup"); // 打印按钮初始化完成
}

void loop() {
    if (digitalRead(button1Pin) == LOW ) { // 如果按钮1引脚为高电平且未被按下
        Serial.println("button1 按下"); // 打印按钮1被按下
    }
    if (digitalRead(button2Pin) == LOW ) { // 如果按钮2引脚为高电平且未被按下
        Serial.println("button2 按下"); // 打印按钮2被按下
    }
    if (digitalRead(button3Pin) == LOW ) { // 如果按钮3引脚为高电平且未被按下
        Serial.println("button3 按下"); // 打印按钮3被按下  
    }

    delay(1000);
}