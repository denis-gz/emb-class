#include <Arduino.h>

#define RED  10
#define BLUE 7

void setup() {
    pinMode(RED, OUTPUT);
    digitalWrite(RED, HIGH);

    pinMode(BLUE, OUTPUT);
    digitalWrite(BLUE, HIGH);
}

void loop() {
    delay(50);
    digitalWrite(RED, LOW);
    delay(50);
    digitalWrite(RED, HIGH);
    delay(50);
    digitalWrite(RED, LOW);
    delay(50);
    digitalWrite(RED, HIGH);
    delay(50);
    digitalWrite(RED, LOW);
    delay(50);
    digitalWrite(RED, HIGH);

    delay(50);
    digitalWrite(BLUE, LOW);
    delay(50);
    digitalWrite(BLUE, HIGH);
    delay(50);
    digitalWrite(BLUE, LOW);
    delay(50);
    digitalWrite(BLUE, HIGH);
    delay(50);
    digitalWrite(BLUE, LOW);
    delay(50);
    digitalWrite(BLUE, HIGH);
}