#include "isr.h"
#include <stdio.h>
#include "esp_attr.h" 

volatile uint16_t flags = 0;

void IRAM_ATTR isr_handler(void *args) {
    flags = (flags + 1) % 4;
}

void isr_install() {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << GPIO_NUM_0),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        printf("Error configuring GPIO: %d\n", ret);
        return;
    }

    ret = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    if (ret != ESP_OK) {
        printf("Error installing ISR service: %d\n", ret);
        return;
    }

    ret = gpio_isr_handler_add(GPIO_NUM_0, isr_handler, NULL);
    if (ret != ESP_OK) {
        printf("Error adding ISR handler: %d\n", ret);
        return;
    }
}