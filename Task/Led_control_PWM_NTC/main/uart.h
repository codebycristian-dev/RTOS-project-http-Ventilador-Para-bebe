#ifndef UART_USER_H
#define UART_USER_H

#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

#define TXD_PIN 1
#define RXD_PIN 3
#define UART_PORT_NUM UART_NUM_0
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE 1024

void init_uart0(void);
char *read_uart(void);

#endif
