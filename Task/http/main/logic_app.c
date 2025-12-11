#include "logic_app.h"
#include "config_app.h"
#include "fan_control.h"
#include "sensor_app.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>
static const char *TAG = "logic_app";

/**
 * Calcula el valor de PWM basado en la temperatura y los umbrales Tmin y Tmax.
 * @param T Temperatura actual.
 * @param Tmin Temperatura mínima para 0% PWM.
 * @param Tmax Temperatura máxima para 100% PWM.
 * @return Valor de PWM (0-100%).
 */
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

/**
 * Verifica si el tiempo actual está dentro del rango especificado.
 * @param current Tiempo actual en minutos desde medianoche.
 * @param start Tiempo de inicio en minutos desde medianoche.
 * @param end Tiempo de fin en minutos desde medianoche.
 * @return true si el tiempo actual está dentro del rango, false en caso contrario.
 */
static bool time_in_range(int current, int start, int end)
{
    if (start <= end)
        return (current >= start && current <= end);
    else
        return (current >= start || current <= end); // cruza medianoche
}

/**
 * Obtiene el día de la semana, hora y minuto actuales.
 * @param weekday Puntero para almacenar el día de la semana (1=lunes, ..., 7=domingo).
 * @param hour Puntero para almacenar la hora actual (0-23).
 * @param minute Puntero para almacenar el minuto actual (0-59).
 */
static void get_time_now(int *weekday, int *hour, int *minute)
{
    time_t now;
    struct tm t;
    time(&now);
    localtime_r(&now, &t);

    // tm_wday: 0=domingo, 1=lunes ... 6=sábado
    *weekday = t.tm_wday;
    if (*weekday == 0)
        *weekday = 7; // hacemos domingo = 7

    *hour = t.tm_hour;
    *minute = t.tm_min;
}
/**
 * Verifica si un día específico está activo en la máscara de días.
 * @param days_mask Máscara de días (bit 0 = lunes, bit 6 = domingo).
 * @param weekday Día de la semana (1=lunes, ..., 7=domingo).
 * @return true si el día está activo, false en caso contrario.
 */
static bool is_day_active(uint8_t days_mask, int weekday)
{
    return (days_mask >> (weekday - 1)) & 1;
}
/**
 * Tarea principal de lógica del ventilador.
 * @param pv Puntero a datos de usuario (no utilizado).
 */
static void logic_task(void *pv)
{
    ESP_LOGI(TAG, "Iniciando tarea lógica del ventilador...");

    while (1)
    {
        fan_config_t *cfg = config_app();

        float temp = sensor_get_temperature();
        bool presence = sensor_get_presence();

        int pwm = 0;

        switch (cfg->mode)
        {
        /* ==========================
           MODO MANUAL
           ========================== */
        case FAN_MODE_MANUAL:
            pwm = cfg->pwm_manual;
            break;

        /* ==========================
           MODO AUTO
           ========================== */
        case FAN_MODE_AUTO:
            if (!presence)
            {
                pwm = 0;
                break;
            }
            pwm = pwm_from_temperature(temp, cfg->Tmin, cfg->Tmax);
            break;

        /* ==========================
           MODO PROGRAMADO
           ========================== */
        case FAN_MODE_PROGRAMMED:
        {
            int weekday, hour_now, minute_now;
            get_time_now(&weekday, &hour_now, &minute_now);

            ESP_LOGI(TAG, "Tiempo actual → día=%d, %02d:%02d",
                     weekday, hour_now, minute_now);

            int now_minutes = hour_now * 60 + minute_now;
            pwm = 0;

            for (int i = 0; i < MAX_REGISTERS; i++)
            {
                fan_register_t *r = &cfg->reg[i];

                ESP_LOGI(TAG,
                         "Reg %d: active=%d daysMask=%02X todayBit=%d "
                         "hora=%02d:%02d → %02d:%02d",
                         i + 1,
                         r->active,
                         r->days,
                         weekday - 1,
                         r->hour_start, r->min_start,
                         r->hour_end, r->min_end);

                // 1. ¿Está activo?
                if (!r->active)
                    continue;

                // 2. ¿Coincide el día?
                if (!is_day_active(r->days, weekday))
                    continue;

                // 3. ¿Coincide la hora?
                int start = r->hour_start * 60 + r->min_start;
                int end = r->hour_end * 60 + r->min_end;

                if (!time_in_range(now_minutes, start, end))
                    continue;

                // 4. Coincide → calcular PWM
                pwm = pwm_from_temperature(temp, r->temp0, r->temp100);

                ESP_LOGI(TAG,
                         "✔ Registro %d coincide → PWM=%d",
                         i + 1, pwm);

                break; // ya se encontró el registro que aplica
            }

            break;
        }

        } // <-- ESTA CIERRA EL SWITCH

        /* ==========================
           APLICAR PWM Y LOG GENERAL
           ========================== */
        fan_set_pwm(pwm);

        ESP_LOGI(TAG,
                 "[Mode=%d] Temp=%.2f Presencia=%d PWM=%d%%",
                 cfg->mode, temp, presence, pwm);

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
/**
 * Inicia la tarea lógica del ventilador.
 */
void logic_app_start(void)
{
    xTaskCreatePinnedToCore(logic_task, "logic_task", 4096, NULL, 5, NULL, 1);
}
