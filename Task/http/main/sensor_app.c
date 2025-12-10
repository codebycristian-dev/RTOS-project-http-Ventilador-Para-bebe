#include "sensor_app.h"
#include "ds18b20_bitbang.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <time.h>

static const char *TAG = "sensor_app";

// -------------------------
// Variables internas
// -------------------------
static DS18B20 sensor;
static float temperature_c = 25.0f;
static bool presence = false;

// -------------------------
// Inicialización
// -------------------------
void sensor_app_init(void)
{
    ESP_LOGI(TAG, "Inicializando sensores...");

    // ----- PIR -----
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << SENSOR_PIR_GPIO,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // ----- DS18B20 (bit-bang sin OWB) -----
    ds18b20_init(&sensor, SENSOR_TEMP_GPIO);
    ESP_LOGI(TAG, "DS18B20 inicializado en GPIO %d", SENSOR_TEMP_GPIO);
}

// -------------------------
// Lectura de temperatura
// -------------------------
float sensor_get_temperature(void)
{
    float temp;

    if (ds18b20_read_temperature(&sensor, &temp))
    {
        temperature_c = temp;
    }
    else
    {
        static int errCount = 0;
        if (errCount++ % 5 == 0)
        {
            ESP_LOGE(TAG, "Error leyendo DS18B20");
        }
    }

    return temperature_c;
}

// -------------------------
// Lectura de presencia (PIR)
// -------------------------
bool sensor_get_presence(void)
{
    presence = gpio_get_level(SENSOR_PIR_GPIO);
    return presence;
}

// -------------------------
// Minutos del día
// -------------------------
int sensor_get_minutes(void)
{
    time_t now;
    struct tm t;

    time(&now);
    localtime_r(&now, &t);

    return t.tm_hour * 60 + t.tm_min;
}

// -------------------------
// Forzar actualización
// -------------------------
void sensor_force_update(void)
{
    sensor_get_temperature();
}
