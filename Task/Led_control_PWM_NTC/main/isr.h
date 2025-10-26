#ifndef ISR_H
#define ISR_H

#include "driver/gpio.h"
#include "esp_attr.h" // 
#include <stdint.h>

#define ESP_INTR_FLAG_DEFAULT 0
#define BUTTON_GPIO GPIO_NUM_4

extern volatile bool print_enabled;

void isr_install(void);
void IRAM_ATTR isr_handler(void *args);

#endif
