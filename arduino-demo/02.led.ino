/**
 * ESP32-S3 按钮控制 LED (Toggle模式)
 * 
 * 接线:
 *   按钮1 -> GPIO 11 (另一端 -> GND)
 *   按钮2 -> GPIO 12 (另一端 -> GND)
 *   按钮3 -> GPIO 13 (另一端 -> GND)
 *   LED红 -> GPIO 15 (串联220Ω, 负极 -> GND)
 *   LED黄 -> GPIO 16 (串联220Ω, 负极 -> GND)
 *   LED绿 -> GPIO 17 (串联220Ω, 负极 -> GND)
 */

#include <Arduino.h>

const int buttonPins[] = {11, 12, 13};
const int ledPins[] = {15, 16, 17};
const int BUTTON_COUNT = 3;
bool lastButtonState[] = {HIGH, HIGH, HIGH};

void setup() {
    Serial.begin(115200);
    
    for (int i = 0; i < BUTTON_COUNT; i++) {
        pinMode(buttonPins[i], INPUT_PULLUP);
        pinMode(ledPins[i], OUTPUT);
    }
    
    Serial.println("LED Toggle Demo Ready");
}

void loop() {
    for (int i = 0; i < BUTTON_COUNT; i++) {
        int state = digitalRead(buttonPins[i]);
        
        if (state == LOW && lastButtonState[i] == HIGH) {
            digitalWrite(ledPins[i], !digitalRead(ledPins[i]));
            
            Serial.print("Button ");
            Serial.print(i + 1);
            Serial.print(" -> LED ");
            Serial.print(i + 1);
            Serial.print(" is ");
            Serial.println(digitalRead(ledPins[i]) ? "ON" : "OFF");
        }
        
        lastButtonState[i] = state;
    }
    
    delay(50);
}