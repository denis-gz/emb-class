#pragma once

#include "handler_base.hpp"

class TimeBased: public HandlerBase
{
public:
    TimeBased();

    void Init();

private:
    void HandlerImpl() override;

    int64_t m_last_inter_time = 0;
};

