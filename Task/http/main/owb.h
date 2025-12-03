#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct OneWireBus OneWireBus;

typedef struct
{
    uint8_t bytes[8];
} OneWireBus_ROMCode;

OneWireBus *owb_rmt_initialize(void *driver_info, int gpio, int tx_channel, int rx_channel);
bool owb_read_byte(OneWireBus *bus, uint8_t *value);
bool owb_write_byte(OneWireBus *bus, uint8_t value);
bool owb_reset(OneWireBus *bus);
bool owb_search_first(OneWireBus *bus, OneWireBus_ROMCode *rom_code);
bool owb_use_crc(OneWireBus *bus, bool enable);
