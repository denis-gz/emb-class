#include "UartReceiver.h"

#include <hal/uart_ll.h>
#include <soc/uart_periph.h>
#include <soc/gpio_num.h>

#include <esp_log.h>
#include <cstring>

#include "common.h"

UartReceiver::UartReceiver(int port_num, uint8_t cmd_len, callback_t cb)
    : m_callback(cb)
    , m_uart_num((uart_port_t) port_num)
{
    const int EVENT_QUEUE_SIZE = 3;

    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(
        m_uart_num,
        UART_HW_FIFO_LEN(m_uart_num) + 1,
        UART_HW_FIFO_LEN(m_uart_num) + 1,
        EVENT_QUEUE_SIZE,
        &m_uart_queue, 0));

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(m_uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(m_uart_num, CONFIG_PIN_UART_TX, CONFIG_PIN_UART_RX, GPIO_NUM_NC, GPIO_NUM_NC));

    uart_intr_config_t intr_config = {
        .intr_enable_mask = UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT | UART_INTR_FRAM_ERR,
        .rx_timeout_thresh = 0,
        .txfifo_empty_intr_thresh = 0,
        .rxfifo_full_thresh = cmd_len,
    };
    ESP_ERROR_CHECK(uart_intr_config(m_uart_num, &intr_config));

    union {
        void (UartReceiver::*member)();
        TaskFunction_t uart_task;
    }
    handler = {
        .member = &UartReceiver::uart_event_task
    };
    xTaskCreate(handler.uart_task, "uart_event_task", 4096, this, 12, &m_uart_task);
    ESP_ERROR_CHECK(uart_enable_rx_intr(m_uart_num));
}

UartReceiver::~UartReceiver()
{
    uart_disable_rx_intr(m_uart_num);
    vTaskDelete(m_uart_task);
    vQueueDelete(m_uart_queue);
    uart_driver_delete(m_uart_num);
}

void UartReceiver::uart_event_task()
{
    uart_event_t event;
    char buf[8];

    while (1) {
        //Waiting for UART event.
        if (xQueueReceive(m_uart_queue, &event, portMAX_DELAY)) {
            bzero(buf, sizeof(buf));
            ESP_LOGI(TAG, "uart event type: %d", event.type);
            switch (event.type) {
                case UART_DATA:
                    uart_read_bytes(m_uart_num, buf, sizeof(buf) - 1, portMAX_DELAY);
                    ESP_LOGI(TAG, "uart data: '%s'", buf);
                    if (!m_callback(buf, sizeof(buf))) {
                        uart_flush_input(m_uart_num);
                    }
                    break;
                case UART_FRAME_ERR:
                    uart_flush_input(m_uart_num);
                    break;
            }
        }
    }
}
