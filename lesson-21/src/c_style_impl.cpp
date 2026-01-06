#include <Arduino.h>

#include "c_style_impl.h"

constexpr const int PIN_LED_1 = 6;
constexpr const int LED_1_ON_DURATION = 10; // ms
constexpr const int LED_1_OFF_DURATION = 200; // ms

constexpr const int PIN_LED_2 = 8;
constexpr const int LED_2_ON_DURATION = 10; // ms
constexpr const int LED_2_OFF_DURATION = 500; // ms

constexpr const int PIN_LED_3 = 12;
constexpr const int LED_3_ON_DURATION = 10; // ms
constexpr const int LED_3_OFF_DURATION = 1000; // ms

static ulong s_time = 0;

#define LED_X_HANDLER(X)												\
	void led_ ## X ## _handler()										\
	{																	\
		static ulong next_on_time = 0;									\
		static ulong next_off_time = 0;									\
																		\
		if (s_time > next_on_time) {									\
			next_off_time = next_on_time + LED_ ## X ## _ON_DURATION;	\
			digitalWrite(PIN_LED_ ## X, LOW);							\
		}																\
		if (s_time > next_off_time) {									\
			next_on_time = next_off_time + LED_ ## X ## _OFF_DURATION;	\
			digitalWrite(PIN_LED_ ## X, HIGH);							\
		}																\
	}																	\

LED_X_HANDLER(1)
LED_X_HANDLER(2)
LED_X_HANDLER(3)

void setup_c_impl()
{
    pinMode(PIN_LED_1, OUTPUT);
    pinMode(PIN_LED_2, OUTPUT);
    pinMode(PIN_LED_3, OUTPUT);
}

void loop_c_impl()
{
    s_time = millis();

	led_1_handler();
    led_2_handler();
    led_3_handler();
}

