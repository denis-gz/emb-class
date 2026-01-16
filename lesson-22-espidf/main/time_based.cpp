#include "time_based.hpp"

#include <freertos/FreeRTOS.h>

constexpr const int64_t DEBOUNCE_GUARD_TIME_US = 50 * 1000;   // 50 ms

TimeBased::TimeBased(): HandlerBase("TimeBased")
{ }

void TimeBased::Init()
{
    HandlerBase::Init(GPIO_INTR_NEGEDGE);
}

void IRAM_ATTR TimeBased::HandlerImpl()
{
    if (m_sample_time > m_last_inter_time + DEBOUNCE_GUARD_TIME_US)
        ++m_counter;

    m_last_inter_time = m_sample_time;
}

