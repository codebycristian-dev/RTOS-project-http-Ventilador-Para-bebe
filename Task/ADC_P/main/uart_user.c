
#include "uart_user.h"

/*initialize uart configuration*/
void init_uart0(void)
{
    /*configuration params uart*/
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    /*install params uart*/
    uart_param_config(UART_PORT_NUM, &uart_config);

    /*gpio set uart outs*/
    uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    /*Install uart drivers*/
    uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0);

    /*Test char for writing in uart*/
    const char *test_str = "Hello, UART!\n";

    /*Write in uart*/
    uart_write_bytes(UART_PORT_NUM, test_str, strlen(test_str));
}

/*reads input from uart and return to buffer*/
char *readuart(void)
{
    char *data = (char *)malloc(UART_BUF_SIZE);
    if (data == NULL)
    {
        ESP_LOGE("UART", "Error: No se pudo asignar memoria");
        return NULL;
    }
    int length = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE, 20 / 1000);
    if (length > 0)
    {
        data[length] = '\0';
    }
    else
    {
        data[0] = '\0';
    }
    return data;
}
