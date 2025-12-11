#include "onewire.h"
/**
 * @brief Inicializa el bus OneWire en el pin especificado.
 * @param bus Puntero a la estructura OneWireBus.
 * @param pin Número de GPIO donde se conectará el bus.
 */
void onewire_init(OneWireBus *bus, gpio_num_t pin)
{
    bus->pin = pin;
    gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_level(pin, 1);
}
/**
 * @brief Realiza un reset en el bus OneWire y detecta la presencia de dispositivos.
 * @param bus Puntero a la estructura OneWireBus.
 * @return true si se detecta un dispositivo, false en caso contrario.
 */
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
/**
 * @brief Escribe un bit en el bus OneWire.
 * @param bus Puntero a la estructura OneWireBus.
 * @param bit Bit a escribir (0 o 1).
 */
void onewire_write_bit(OneWireBus *bus, int bit)
{
    gpio_set_level(bus->pin, 0);
    esp_rom_delay_us(bit ? 6 : 60);
    gpio_set_level(bus->pin, 1);
    esp_rom_delay_us(bit ? 64 : 10);
}
/**
 * @brief Lee un bit del bus OneWire.
 * @param bus Puntero a la estructura OneWireBus.
 * @return Bit leído (0 o 1).
 */
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
/**
 * @brief Escribe un byte en el bus OneWire.
 * @param bus Puntero a la estructura OneWireBus.
 * @param byte Byte a escribir.
 */
void onewire_write_byte(OneWireBus *bus, uint8_t byte)
{
    for (int i = 0; i < 8; i++)
    {
        onewire_write_bit(bus, byte & 1);
        byte >>= 1;
    }
}
/**
 * @brief Lee un byte del bus OneWire.
 * @param bus Puntero a la estructura OneWireBus.
 * @return Byte leído.
 */
uint8_t onewire_read_byte(OneWireBus *bus)
{
    uint8_t value = 0;
    for (int i = 0; i < 8; i++)
    {
        value |= (onewire_read_bit(bus) << i);
    }
    return value;
}
