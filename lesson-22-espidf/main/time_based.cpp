#include "time_based.hpp"


TimeBased::TimeBased(): HandlerBase("TimeBased")
{ }

void TimeBased::Init()
{
    HandlerBase::Init(GPIO_INTR_NEGEDGE);
}

void TimeBased::HandlerImpl()
{
    if (m_sample_time > m_last_inter_time + DEBOUNCE_GUARD_TIME_US)
        ++m_counter;

    m_last_inter_time = m_sample_time;
}

