#include <Arduino.h>

#include "cpp_style_impl.h"

static LED_handler led_1_handler(6, 10, 200);
static LED_handler led_2_handler(8, 10, 500);
static LED_handler led_3_handler(12, 10, 1000);

LED_handler::LED_handler(uint pin, uint on_duration, uint off_duration)
	: pin(pin)
	, on_duration(on_duration)
	, off_duration(off_duration)
{ }

void LED_handler::schedule_work(ulong now_time) {
	m_its_time = false;

	if (now_time > m_next_on_time) {
		m_next_off_time = m_next_on_time + on_duration;
		m_logical_state = true;
		m_its_time = true;
	}
	if (now_time > m_next_off_time) {
		m_next_on_time = m_next_off_time + off_duration;
		m_logical_state = false;
		m_its_time = true;
	}
}

void LED_handler::do_work() {
	if (m_its_time) {
		// The pin uses pull-up logic
		digitalWrite(pin, m_logical_state ? LOW : HIGH);
	}
}

void setup_cpp_impl()
{
    pinMode(led_1_handler.pin, OUTPUT);
    pinMode(led_2_handler.pin, OUTPUT);
    pinMode(led_3_handler.pin, OUTPUT);
}

void loop_cpp_impl()
{
    ulong now_time = millis();

    // Schedule work first, to avoid accumulating offset errors
    led_1_handler.schedule_work(now_time);
    led_2_handler.schedule_work(now_time);
    led_3_handler.schedule_work(now_time);

    // Now do actual work
    led_1_handler.do_work();
    led_2_handler.do_work();
    led_3_handler.do_work();
}