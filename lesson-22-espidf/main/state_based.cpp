#include "state_based.hpp"

// How much time a pin should have the HIGH level to clear button depressed state
constexpr const int64_t HIGH_STATE_GUARD_INTERVAL_US = 10 * 1000LL;

StateBased::StateBased(): HandlerBase("StateBased")
{ }

void StateBased::Init()
{
    HandlerBase::Init(GPIO_INTR_ANYEDGE);
}

void StateBased::HandlerImpl()
{
    if (m_level == 0
        && m_depress_time   // Exclude consequtive depress or release events
        && m_release_time
        && m_depress_time - m_release_time > HIGH_STATE_GUARD_INTERVAL_US) {
        ++m_counter;
    }
}
