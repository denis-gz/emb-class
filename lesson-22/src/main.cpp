#include <Arduino.h>

constexpr const int PIN_LED_1 = 6;
constexpr const int PIN_LED_2 = 3;
constexpr const int PIN_BUTTON = 4;

constexpr const int LED_1_ON_DURATION = 20; // ms
constexpr const int LED_1_OFF_DURATION = 500; // ms

constexpr const int LED_2_ON_DURATION = 200; // ms
constexpr const int LED_2_OFF_DURATION = 500; // ms

volatile bool g_button_pressed = false;

ulong g_time = 0;

void IRAM_ATTR buttonHandler()
{
    //if (digitalRead(PIN_BUTTON) == LOW) { 
        g_button_pressed = true;
    //}
}

void setup()
{
    pinMode(PIN_LED_1, OUTPUT);
    pinMode(PIN_LED_2, OUTPUT);
	//pinMode(PIN_BUTTON, INPUT_PULLUP);
	pinMode(PIN_BUTTON, INPUT);

    attachInterrupt(PIN_BUTTON, buttonHandler, FALLING);
}

// On time 
void led_1_handler()
{
    static ulong next_on_time = 0;
    static ulong next_off_time = 0;

    if (g_time > next_on_time) {
        next_off_time = next_on_time + LED_1_ON_DURATION;
        digitalWrite(PIN_LED_1, LOW);
    }
    if (g_time > next_off_time) {
        next_on_time = next_off_time + LED_1_OFF_DURATION;
        digitalWrite(PIN_LED_1, HIGH);
    }
}

void led_2_handler()
{
    static ulong next_on_time = 0;
    static ulong next_off_time = 0;

    if (g_button_pressed) {
        g_button_pressed = false;
        if (next_off_time == ULONG_MAX) {
            next_on_time = 0;
            next_off_time = 0;
        }
        else {
            next_off_time = ULONG_MAX;
            next_on_time = ULONG_MAX;
            digitalWrite(PIN_LED_2, HIGH);
        }
    }

    if (g_time > next_on_time) {
        next_off_time = next_on_time + LED_2_ON_DURATION;
        digitalWrite(PIN_LED_2, LOW);
    }
    if (g_time > next_off_time) {
        next_on_time = next_off_time + LED_2_OFF_DURATION;
        digitalWrite(PIN_LED_2, HIGH);
    }
}

void loop()
{
    g_time = millis();

	led_1_handler();
    led_2_handler();
}
