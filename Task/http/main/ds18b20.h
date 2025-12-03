#pragma once

#include "owb.h"

typedef struct DS18B20_Info DS18B20_Info;

DS18B20_Info *ds18b20_malloc(void);
void ds18b20_free(DS18B20_Info *info);
void ds18b20_init(DS18B20_Info *info, OneWireBus *bus, OneWireBus_ROMCode rom_code);
void ds18b20_use_crc(DS18B20_Info *info, bool use_crc);
void ds18b20_set_resolution(DS18B20_Info *info, int resolution_bits);
void ds18b20_convert_all(OneWireBus *bus);
void ds18b20_wait_for_conversion(DS18B20_Info *info);
bool ds18b20_read_temp(DS18B20_Info *info, float *value);
