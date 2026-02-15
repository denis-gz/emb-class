#pragma once

#include <functional>
#include <driver/uart.h>

class UartReceiver
{
public:
    using callback_t = std::function<bool(const char* data, size_t len)>;

    UartReceiver(int port_num, uint8_t cmd_len, callback_t cb);
    ~UartReceiver();

private:
    callback_t m_callback;
    uart_port_t m_uart_num = {};

    QueueHandle_t m_uart_queue = nullptr;
    TaskHandle_t m_uart_task = nullptr;

    void uart_event_task();
};
