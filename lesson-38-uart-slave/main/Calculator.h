#pragma once

#include "S7_Display.h"
#include "UartReceiver.h"

class Calculator
{
    enum state_t {
        FirstArg,
        Operator,
        SecondArg,
        Result,
    };

    enum operator_t {
        Add,
        Subtract,
        Multiply,
        Divide,
    };

    const int OpCount = 4;

public:
    Calculator(int num_digits);
    ~Calculator();

private:
    const int m_num_digits;
    int* m_digits = nullptr;
    int m_dig_index = 0;

    S7_Display m_display;
    UartReceiver m_receiver;

    int m_arg_1 = 0;
    int m_arg_2 = 0;
    operator_t m_operator = {};

    state_t m_state = {};

    void reset();
    void clear_digits();
    void calculate(int& arg);
    int  get_result();
    void update_display();
    bool on_command_received(const char* command, size_t len);
    void on_rotate(bool increase);
    void on_click();
};

