/**
 * ESP32-S3 按钮示例
 * 
 * 接线: 按钮一端接GPIO，另一端接GND
 *   按钮1 -> GPIO 11
 *   按钮2 -> GPIO 12
 *   按钮3 -> GPIO 13
 */

#include <Arduino.h>

#define BUTTON1_PIN  11
#define BUTTON2_PIN  12
#define BUTTON3_PIN  13

void setup() {
    Serial.begin(115200);
    
    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);
    pinMode(BUTTON3_PIN, INPUT_PULLUP);
    
    Serial.println("Button Demo Ready");
}

void loop() {
    if (digitalRead(BUTTON1_PIN) == LOW) {
        Serial.println("Button 1 pressed");
    }
    if (digitalRead(BUTTON2_PIN) == LOW) {
        Serial.println("Button 2 pressed");
    }
    if (digitalRead(BUTTON3_PIN) == LOW) {
        Serial.println("Button 3 pressed");
    }
    delay(100);
}