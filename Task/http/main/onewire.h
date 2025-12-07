#ifndef ONEWIRE_H
#define ONEWIRE_H

#include "driver/gpio.h"
#include "esp_rom_sys.h"

typedef struct
{
    gpio_num_t pin;
} OneWireBus;

void onewire_init(OneWireBus *bus, gpio_num_t pin);
bool onewire_reset(OneWireBus *bus);
void onewire_write_bit(OneWireBus *bus, int bit);
int onewire_read_bit(OneWireBus *bus);
void onewire_write_byte(OneWireBus *bus, uint8_t byte);
uint8_t onewire_read_byte(OneWireBus *bus);

#endif
