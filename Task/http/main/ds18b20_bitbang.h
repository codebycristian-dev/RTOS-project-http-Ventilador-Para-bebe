#ifndef DS18B20_BITBANG_H
#define DS18B20_BITBANG_H

#include "onewire.h"
/**
 * @brief Estructura para el sensor DS18B20.
 */
typedef struct
{
    OneWireBus bus;
} DS18B20;

void ds18b20_init(DS18B20 *sensor, gpio_num_t pin);
bool ds18b20_read_temperature(DS18B20 *sensor, float *temp_c);

#endif
