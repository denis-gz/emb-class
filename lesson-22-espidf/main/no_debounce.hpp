#pragma once

#include "handler_base.hpp"

class NoDebounce: public HandlerBase
{
public:
    NoDebounce();

    void Init();

private:
    void HandlerImpl() override;
};
