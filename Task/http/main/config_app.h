#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Modos de funcionamiento del ventilador.
     */
    typedef enum
    {
        FAN_MODE_MANUAL = 0,
        FAN_MODE_AUTO = 1,
        FAN_MODE_PROGRAMMED = 2
    } fan_mode_t;

    /**
     * @brief Estructura de un registro programado del ventilador.
     */
    typedef struct
    {
        int active;     // 0=off, 1=on
        int hour_start; // 0-23
        int min_start;  // 0-59
        int hour_end;   // 0-23
        int min_end;    // 0-59
        float temp0;    // temp para 0% PWM
        float temp100;  // temp para 100% PWM
    } fan_register_t;

    /**
     * @brief Estructura completa de configuración.
     */
    typedef struct
    {
        fan_mode_t mode;       // Modo actual
        int pwm_manual;        // 0–100 %
        float Tmin;            // Temp mínima (auto)
        float Tmax;            // Temp máxima (auto)
        fan_register_t reg[3]; // Tres registros programados
    } fan_config_t;

    /**
     * @brief Inicializa NVS y carga toda la configuración del ventilador.
     */
    void config_app_init(void);

    /**
     * @brief Guarda toda la configuración en NVS.
     */
    void config_app_save(void);

    /**
     * @brief Devuelve referencia a la configuración global.
     */
    fan_config_t *config_app(void);

#ifdef __cplusplus
}
#endif
