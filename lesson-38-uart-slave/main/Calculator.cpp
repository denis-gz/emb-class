#include "Calculator.h"

#include <freertos/FreeRTOS.h>
#include <cstring>

#include <esp_log.h>

#include "sdkconfig.h"
#include "common.h"

#define ENC_CWS "ENC_CWS"
#define ENC_CCW "ENC_CCW"
#define ENC_KEY "ENC_KEY"
#define CMD_LEN 7

namespace ph = std::placeholders;

Calculator::Calculator(int num_digits)
    : m_num_digits(num_digits)
    , m_display(num_digits)
    , m_receiver(CONFIG_UART_NUM, CMD_LEN, std::bind(&Calculator::on_command_received, this, ph::_1, ph::_2))
{
    m_digits = new int[num_digits];
    m_display.Start();

    reset();
    update_display();
}

Calculator::~Calculator()
{
    delete m_digits;
}

void Calculator::reset()
{
    m_arg_1 = 0;
    m_arg_2 = 0;
    m_dig_index = 0;
    m_operator = {};
    m_state = FirstArg;
    clear_digits();
}

void Calculator::clear_digits()
{
    memset(m_digits, 0, m_num_digits * sizeof(int));
}

void Calculator::calculate(int& arg)
{
    arg = 0;
    for (int i = 0, n = 1; i < m_num_digits; ++i, n *= 10) {
        arg += n * m_digits[i];
    }
}

int Calculator::get_result()
{
    int result = 0;
    switch (m_operator) {
        case Add:
            result = m_arg_1 + m_arg_2;
            break;
        case Subtract:
            result = m_arg_1 - m_arg_2;
            break;
        case Multiply:
            result = m_arg_1 * m_arg_2;
            break;
        case Divide:
            result = m_arg_1 / m_arg_2;
            break;
    }
    return result;
}

void Calculator::update_display()
{
    switch (m_state) {
        case FirstArg:
            calculate(m_arg_1);
            m_display.Print(m_arg_1, m_dig_index);
            break;
        case SecondArg:
            calculate(m_arg_2);
            m_display.Print(m_arg_2, m_dig_index);
            break;
        case Operator: {
            switch (m_operator) {
                case Add:
                    m_display.Print("ADD");
                    break;
                case Subtract:
                    m_display.Print("SUB");
                    break;
                case Multiply:
                    m_display.Print("MUL");
                    break;
                case Divide:
                    m_display.Print("DIV");
                    break;
            }
            break;
        }
        case Result: {
            if (m_operator == Divide && m_arg_2 == 0) {
                m_display.Print("ERR");
            }
            else {
                int result = get_result();
                m_display.Print(result, result ? -1 : 0);
            }
            break;
        }
    }
}

bool Calculator::on_command_received(const char* command, size_t size)
{
    if (std::strncmp(command, ENC_CCW, size) == 0) {
        on_rotate(false);
    }
    else if (std::strncmp(command, ENC_CWS, size) == 0) {
        on_rotate(true);
    }
    else if (std::strncmp(command, ENC_KEY, size) == 0) {
        on_click();
    }
    else {
        return false;
    }
    return true;
}

void Calculator::on_rotate(bool increase)
{
    switch (m_state) {
        case FirstArg:
        case SecondArg: {
            if (increase) {
                int j = m_digits[m_dig_index];
                m_digits[m_dig_index] = ++j % 10;
            }
            else {
                int j = m_digits[m_dig_index] + 10;
                m_digits[m_dig_index] = --j % 10;
            }
            update_display();
            break;
        }
        case Operator: {
            if (increase) {
                int j = m_operator;
                m_operator = static_cast<operator_t>(++j % OpCount);
            }
            else {
                int j = m_operator + OpCount;
                m_operator = static_cast<operator_t>(--j % OpCount);
            }
            update_display();
            break;
        }
        default:
            break;
    }
}

void Calculator::on_click()
{
    switch (m_state) {
        case FirstArg:
            if (++m_dig_index >= m_num_digits)
                m_state = Operator;
            update_display();
            break;
        case Operator:
            m_dig_index = 0;
            m_state = SecondArg;
            clear_digits();
            update_display();
            break;
        case SecondArg:
            if (++m_dig_index >= m_num_digits)
                m_state = Result;
            update_display();
            break;
        case Result:
            reset();
            update_display();
            break;
    }
}
