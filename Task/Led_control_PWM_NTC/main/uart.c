#include "uart.h"

void init_uart0(void)
{
    uart_config_t config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_PORT_NUM, &config);
    uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE, 0, 0, NULL, 0);
}

char *read_uart(void)
{
    static char data[UART_BUF_SIZE];
    int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);
    if (len > 0)
    {
        data[len] = '\0';
        return data;
    }
    return NULL;
}
