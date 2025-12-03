#include "logic_app.h"
#include "config_app.h"
#include "fan_control.h"
#include "sensor_app.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "logic_app";

// Convertir temperatura en PWM proporcional
static int pwm_from_temperature(float T, float Tmin, float Tmax)
{
    if (T <= Tmin)
        return 0;
    if (T >= Tmax)
        return 100;

    float p = ((T - Tmin) / (Tmax - Tmin)) * 100.0f;
    if (p < 0)
        p = 0;
    if (p > 100)
        p = 100;

    return (int)p;
}

// Verifica si la hora actual está dentro de un intervalo
static bool time_in_range(int current, int start, int end)
{
    if (start <= end)
        return (current >= start && current <= end);
    else
        return (current >= start || current <= end); // cruza medianoche
}

static void logic_task(void *pv)
{
    ESP_LOGI(TAG, "Iniciando tarea lógica del ventilador...");

    while (1)
    {
        fan_config_t *cfg = config_app();

        float temp = sensor_get_temperature();
        bool presence = sensor_get_presence();
        int minutes = sensor_get_minutes();

        int pwm = 0;

        switch (cfg->mode)
        {
        case FAN_MODE_MANUAL:
            pwm = cfg->pwm_manual;
            break;

        case FAN_MODE_AUTO:
            if (!presence)
            {
                pwm = 0;
                break;
            }
            pwm = pwm_from_temperature(temp, cfg->Tmin, cfg->Tmax);
            break;

        case FAN_MODE_PROGRAMMED:
            if (!presence)
            {
                pwm = 0;
                break;
            }

            pwm = 0;
            for (int i = 0; i < 3; i++)
            {
                if (!cfg->reg[i].active)
                    continue;

                int start = cfg->reg[i].hour_start * 60 + cfg->reg[i].min_start;
                int end = cfg->reg[i].hour_end * 60 + cfg->reg[i].min_end;

                if (time_in_range(minutes, start, end))
                {
                    pwm = pwm_from_temperature(temp,
                                               cfg->reg[i].temp0,
                                               cfg->reg[i].temp100);
                    break;
                }
            }
            break;
        }

        fan_set_pwm(pwm);

        ESP_LOGI(TAG,
                 "[Mode=%d] Temp=%.2f Presencia=%d PWM=%d%%",
                 cfg->mode, temp, presence, pwm);

        vTaskDelay(pdMS_TO_TICKS(1000)); // cada 1 segundo
    }
}

void logic_app_start(void)
{
    xTaskCreatePinnedToCore(logic_task, "logic_task", 4096, NULL, 5, NULL, 1);
}
