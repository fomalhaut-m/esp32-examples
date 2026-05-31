#include <Arduino.h>

#define RED_LED_PIN   48
#define YELLOW_LED_PIN  2
#define GREEN_LED_PIN   1

#define CHASE_DELAY  300

void setup() {
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
}

void loop() {
  digitalWrite(RED_LED_PIN, HIGH);
  delay(CHASE_DELAY);
  digitalWrite(RED_LED_PIN, LOW);
  delay(50);
  
  digitalWrite(YELLOW_LED_PIN, HIGH);
  delay(CHASE_DELAY);
  digitalWrite(YELLOW_LED_PIN, LOW);
  delay(50);
  
  digitalWrite(GREEN_LED_PIN, HIGH);
  delay(CHASE_DELAY);
  digitalWrite(GREEN_LED_PIN, LOW);
  delay(50);
}
