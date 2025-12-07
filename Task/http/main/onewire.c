#include "onewire.h"

void onewire_init(OneWireBus *bus, gpio_num_t pin)
{
    bus->pin = pin;
    gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_level(pin, 1);
}

bool onewire_reset(OneWireBus *bus)
{
    gpio_set_direction(bus->pin, GPIO_MODE_INPUT_OUTPUT_OD);

    gpio_set_level(bus->pin, 0);
    esp_rom_delay_us(480);

    gpio_set_level(bus->pin, 1);
    esp_rom_delay_us(70);

    int presence = gpio_get_level(bus->pin);
    esp_rom_delay_us(410);

    return (presence == 0); // 0 = sensor responde OK
}

void onewire_write_bit(OneWireBus *bus, int bit)
{
    gpio_set_level(bus->pin, 0);
    esp_rom_delay_us(bit ? 6 : 60);
    gpio_set_level(bus->pin, 1);
    esp_rom_delay_us(bit ? 64 : 10);
}

int onewire_read_bit(OneWireBus *bus)
{
    gpio_set_level(bus->pin, 0);
    esp_rom_delay_us(6);

    gpio_set_level(bus->pin, 1);
    esp_rom_delay_us(9);

    int bit = gpio_get_level(bus->pin);
    esp_rom_delay_us(55);

    return bit;
}

void onewire_write_byte(OneWireBus *bus, uint8_t byte)
{
    for (int i = 0; i < 8; i++)
    {
        onewire_write_bit(bus, byte & 1);
        byte >>= 1;
    }
}

uint8_t onewire_read_byte(OneWireBus *bus)
{
    uint8_t value = 0;
    for (int i = 0; i < 8; i++)
    {
        value |= (onewire_read_bit(bus) << i);
    }
    return value;
}
