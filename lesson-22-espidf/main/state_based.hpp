#pragma once

#include "handler_base.hpp"

class StateBased: public HandlerBase
{
public:
    StateBased();

    void Init();

private:
    void HandlerImpl() override;
};

