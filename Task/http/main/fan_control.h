#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Inicializa el PWM para controlar el ventilador.
     */
    void fan_control_init(void);

    /**
     * @brief Ajusta el porcentaje de PWM para el ventilador.
     *
     * @param percent Valor entre 0 y 100.
     */
    void fan_set_pwm(int percent);

    /**
     * @brief Devuelve el PWM aplicado actualmente (0–100%).
     */
    int fan_get_current_pwm(void);

#ifdef __cplusplus
}
#endif
