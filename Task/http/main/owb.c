#include "owb.h"
#include <stdlib.h>

struct OneWireBus
{
    int gpio;
    bool use_crc;
};

OneWireBus *owb_rmt_initialize(void *driver_info, int gpio, int tx_channel, int rx_channel)
{
    OneWireBus *bus = calloc(1, sizeof(OneWireBus));
    bus->gpio = gpio;
    return bus;
}

bool owb_reset(OneWireBus *bus)
{
    return true;
}

bool owb_read_byte(OneWireBus *bus, uint8_t *value)
{
    *value = 0x50; // Dummy value
    return true;
}

bool owb_write_byte(OneWireBus *bus, uint8_t value)
{
    return true;
}

bool owb_search_first(OneWireBus *bus, OneWireBus_ROMCode *rom_code)
{
    for (int i = 0; i < 8; i++)
        rom_code->bytes[i] = i;
    return true;
}

bool owb_use_crc(OneWireBus *bus, bool enable)
{
    bus->use_crc = enable;
    return true;
}
