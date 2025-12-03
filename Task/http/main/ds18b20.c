#include "ds18b20.h"
#include <stdlib.h>

struct DS18B20_Info
{
    OneWireBus *bus;
    bool crc;
    int resolution_bits;
};

DS18B20_Info *ds18b20_malloc(void)
{
    return calloc(1, sizeof(DS18B20_Info));
}

void ds18b20_free(DS18B20_Info *info)
{
    free(info);
}

void ds18b20_init(DS18B20_Info *info, OneWireBus *bus, OneWireBus_ROMCode rom_code)
{
    info->bus = bus;
}

void ds18b20_use_crc(DS18B20_Info *info, bool use_crc)
{
    info->crc = use_crc;
}

void ds18b20_set_resolution(DS18B20_Info *info, int resolution_bits)
{
    info->resolution_bits = resolution_bits;
}

void ds18b20_convert_all(OneWireBus *bus) {}

void ds18b20_wait_for_conversion(DS18B20_Info *info) {}

bool ds18b20_read_temp(DS18B20_Info *info, float *value)
{
    *value = 25.0; // dummy temperature
    return true;
}
