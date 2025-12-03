#include "sensor_app.h"
#include "owb.h"
#include "ds18b20.h"
#include <stdio.h>
#include <time.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "owb_rmt.h"

static const char *TAG = "sensor_app";

// Variables internas
static float temperature_c = 25.0f; // valor por defecto
static bool presence = false;

// Manipulador OneWire y DS18B20
static OneWireBus *owb = NULL;
static owb_rmt_driver_info rmt_info;
static DS18B20_Info *ds18b20 = NULL;

void sensor_app_init(void)
{
    ESP_LOGI(TAG, "Inicializando sensores...");

    // ---------------- PIR ----------------
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << SENSOR_PIR_GPIO,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // ---------------- DS18B20 (versión minimalista) ----------------
    owb = owb_rmt_initialize(&rmt_info, SENSOR_TEMP_GPIO, 0, 0);
    if (!owb)
    {
        ESP_LOGE(TAG, "Error inicializando OWB");
        return;
    }

    OneWireBus_ROMCode rom;
    owb_search_first(owb, &rom);

    ds18b20 = ds18b20_malloc();
    ds18b20_init(ds18b20, owb, rom);

    ESP_LOGI(TAG, "Sensor DS18B20 minimalista inicializado.");
}

float sensor_get_temperature(void)
{
    // La librería dummy devuelve 25°C fijo
    ds18b20_read_temp(ds18b20, &temperature_c);
    return temperature_c;
}

bool sensor_get_presence(void)
{
    presence = gpio_get_level(SENSOR_PIR_GPIO);
    return presence;
}

int sensor_get_minutes(void)
{
    time_t now;
    struct tm t;

    time(&now);
    localtime_r(&now, &t);

    return t.tm_hour * 60 + t.tm_min;
}

void sensor_force_update(void)
{
    ds18b20_read_temp(ds18b20, &temperature_c);
}
