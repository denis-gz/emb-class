#include "no_debounce.hpp"


NoDebounce::NoDebounce(): HandlerBase("NoDebounce")
{ }

void NoDebounce::Init()
{
    HandlerBase::Init(GPIO_INTR_NEGEDGE);
}

void NoDebounce::HandlerImpl()
{
    ++m_counter;
}
