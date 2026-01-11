#pragma once

#include <Arduino.h>

class LED_handler
{
public:
	LED_handler(uint pin, uint on_duration, uint off_duration);

	void schedule_work(ulong now_time);
	void do_work();

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