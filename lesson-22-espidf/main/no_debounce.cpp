#include "no_debounce.hpp"

#include <freertos/FreeRTOS.h>

NoDebounce::NoDebounce(): HandlerBase("NoDebounce")
{ }

void NoDebounce::Init()
{
    HandlerBase::Init(GPIO_INTR_NEGEDGE);
}

void IRAM_ATTR NoDebounce::HandlerImpl()
{
    ++m_counter;
}
