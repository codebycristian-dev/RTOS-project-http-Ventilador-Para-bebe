#include "driver/uart.h"
#include "esp_log.h"
#include "string.h"
#include "driver/uart.h"
#include "string.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TXD_PIN 1
#define RXD_PIN 3
#define UART_PORT_NUM UART_NUM_0
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE (1024)
#define TIME_OF_READ 20/1000


void init_uart(void);
char *readuart(void);
void uart_send(const char *mensaje);
bool uart_available(char *buffer, size_t len);