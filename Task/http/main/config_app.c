#include "config_app.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "config_app";
static const char *NS = "fan"; // Namespace fan

static fan_config_t cfg; // Configuración global del ventilador

fan_config_t *config_app(void)
{
    return &cfg;
}

static void load_register(nvs_handle_t h, const char *key, fan_register_t *r)
{
    size_t size = sizeof(fan_register_t);
    esp_err_t err = nvs_get_blob(h, key, r, &size);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Registro %s no encontrado, usando valores por defecto", key);
        r->active = 0;
        r->hour_start = 0;
        r->min_start = 0;
        r->hour_end = 0;
        r->min_end = 0;
        r->temp0 = 24.0;
        r->temp100 = 30.0;
    }
}

static void save_register(nvs_handle_t h, const char *key, fan_register_t *r)
{
    nvs_set_blob(h, key, r, sizeof(fan_register_t));
}

void config_app_init(void)
{
    ESP_LOGI(TAG, "Inicializando configuración del ventilador...");

    nvs_handle_t h;
    esp_err_t err = nvs_open(NS, NVS_READWRITE, &h);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error abriendo NVS (%s)", esp_err_to_name(err));
        return;
    }

    // --------- CARGAR MODO ---------
    uint32_t mode = 0;
    if (nvs_get_u32(h, "mode", &mode) != ESP_OK)
    {
        mode = FAN_MODE_MANUAL;
    }
    cfg.mode = mode;

    // --------- PWM MANUAL ---------
    if (nvs_get_i32(h, "pwm_manual", &cfg.pwm_manual) != ESP_OK)
    {
        cfg.pwm_manual = 0;
    }

    // --------- AUTO Tmin / Tmax ---------
    // ----- Cargar Tmin -----
    size_t size = sizeof(float);
    err = nvs_get_blob(h, "tmin", &cfg.Tmin, &size);
    if (err != ESP_OK)
    {
        cfg.Tmin = 24.0f; // Valor por defecto
    }

    // ----- Cargar Tmax -----
    size = sizeof(float);
    err = nvs_get_blob(h, "tmax", &cfg.Tmax, &size);
    if (err != ESP_OK)
    {
        cfg.Tmax = 30.0f; // Valor por defecto
    }

    // --------- REGISTROS 1..3 ---------
    load_register(h, "reg1", &cfg.reg[0]);
    load_register(h, "reg2", &cfg.reg[1]);
    load_register(h, "reg3", &cfg.reg[2]);

    nvs_close(h);

    ESP_LOGI(TAG, "Configuración cargada correctamente.");
}

void config_app_save(void)
{
    ESP_LOGI(TAG, "Guardando configuración del ventilador...");

    nvs_handle_t h;
    esp_err_t err = nvs_open(NS, NVS_READWRITE, &h);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error abriendo NVS (%s)", esp_err_to_name(err));
        return;
    }

    nvs_set_u32(h, "mode", cfg.mode);
    nvs_set_i32(h, "pwm_manual", cfg.pwm_manual);
    nvs_set_blob(h, "tmin", &cfg.Tmin, sizeof(float));
    nvs_set_blob(h, "tmax", &cfg.Tmax, sizeof(float));

    save_register(h, "reg1", &cfg.reg[0]);
    save_register(h, "reg2", &cfg.reg[1]);
    save_register(h, "reg3", &cfg.reg[2]);

    nvs_commit(h);
    nvs_close(h);

    ESP_LOGI(TAG, "Configuración guardada correctamente.");
}
