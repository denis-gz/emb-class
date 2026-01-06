#pragma once

#include <Arduino.h>

class LED_handler
{
public:
	LED_handler(uint pin, uint on_duration, uint off_duration)
		: pin(pin)
		, on_duration(on_duration)
		, off_duration(off_duration)
	{ }

	void schedule_work(ulong now_time) {
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

	void do_work() {
		if (m_its_time) {
			// The pin uses pull-up logic
			digitalWrite(pin, m_logical_state ? LOW : HIGH);
		}
	}

public:	
	const uint pin;
	const uint on_duration;
	const uint off_duration;

private:
	ulong m_next_on_time = 0;
	ulong m_next_off_time = 0;

	bool m_its_time = false;
	bool m_logical_state = false;
};

void setup_cpp_impl();
void loop_cpp_impl();