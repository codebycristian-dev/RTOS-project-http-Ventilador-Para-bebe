#ifndef ISR_H
#define ISR_H

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_attr.h" 

#define ESP_INTR_FLAG_DEFAULT 0

extern volatile uint16_t flags;

void isr_install(void);
void IRAM_ATTR isr_handler(void *args);

#endif // ISR_H