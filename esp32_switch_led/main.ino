#include <Arduino.h>

#define RED_LED_PIN       48
#define YELLOW_LED_PIN     2
#define GREEN_LED_PIN      1

#define RED_SWITCH_PIN    10
#define YELLOW_SWITCH_PIN 11
#define GREEN_SWITCH_PIN  12

void setup() {
  Serial.begin(115200);
  
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  
  pinMode(RED_SWITCH_PIN, INPUT_PULLUP);
  pinMode(YELLOW_SWITCH_PIN, INPUT_PULLUP);
  pinMode(GREEN_SWITCH_PIN, INPUT_PULLUP);
  
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
}

void loop() {
  bool redState = (digitalRead(RED_SWITCH_PIN) == LOW);
  bool yellowState = (digitalRead(YELLOW_SWITCH_PIN) == LOW);
  bool greenState = (digitalRead(GREEN_SWITCH_PIN) == LOW);
  
  digitalWrite(RED_LED_PIN, redState ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, yellowState ? HIGH : LOW);
  digitalWrite(GREEN_LED_PIN, greenState ? HIGH : LOW);
  
  Serial.printf("R:%d Y:%d G:%d\n", redState, yellowState, greenState);
  delay(50);
}
