#include "isr.h"
#include <stdio.h>
#include "esp_attr.h"

volatile bool print_enabled = true; // Inicia mostrando temperatura

void IRAM_ATTR isr_handler(void *args)
{
    print_enabled = !print_enabled;
}

void isr_install(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE};
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(BUTTON_GPIO, isr_handler, NULL);
    printf("ISR instalada en GPIO4\n");
}
