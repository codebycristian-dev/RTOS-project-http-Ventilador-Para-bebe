#include "ds18b20_bitbang.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/**
 * @brief Inicializa el sensor DS18B20 en el pin especificado.
 * @param sensor Puntero a la estructura DS18B20.
 * @param pin Número de GPIO donde está conectado el sensor.
 * /
 */
void ds18b20_init(DS18B20 *sensor, gpio_num_t pin)
{
    onewire_init(&sensor->bus, pin);
}
/**
 *  @brief Lee la temperatura del sensor DS18B20.
 * @param sensor Puntero a la estructura DS18B20.
 * @param temp_c Puntero a la variable donde se almacenará la temperatura en grados Celsius.
 * @return true si la lectura fue exitosa, false en caso contrario.
 */
bool ds18b20_read_temperature(DS18B20 *sensor, float *temp_c)
{
    if (!onewire_reset(&sensor->bus))
    {
        return false;
    }

    onewire_write_byte(&sensor->bus, 0xCC); // SKIP ROM
    onewire_write_byte(&sensor->bus, 0x44); // CONVERT T

    vTaskDelay(pdMS_TO_TICKS(750)); // max conversion time

    if (!onewire_reset(&sensor->bus))
    {
        return false;
    }

    onewire_write_byte(&sensor->bus, 0xCC); // SKIP ROM
    onewire_write_byte(&sensor->bus, 0xBE); // READ SCRATCHPAD

    uint8_t lsb = onewire_read_byte(&sensor->bus);
    uint8_t msb = onewire_read_byte(&sensor->bus);

    int16_t raw = (msb << 8) | lsb;
    *temp_c = raw / 16.0f;

    return true;
}
