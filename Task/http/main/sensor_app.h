#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

// ------------------------------------------------------
//  CONFIGURACIÓN DE PINES
// ------------------------------------------------------
#define SENSOR_TEMP_GPIO 4 // DS18B20
#define SENSOR_PIR_GPIO 27 // PIR HC-SR501

    // ------------------------------------------------------
    //  INTERFAZ PÚBLICA
    // ------------------------------------------------------

    /**
     * @brief Inicializa el sensor DS18B20 y el PIR.
     */
    void sensor_app_init(void);

    /**
     * @brief Devuelve la última temperatura medida (°C)
     */
    float sensor_get_temperature(void);

    /**
     * @brief Devuelve true si hay presencia detectada por el PIR.
     */
    bool sensor_get_presence(void);

    /**
     * @brief Devuelve minutos actuales del día (0–1440),
     *        útil para modo programado.
     */
    int sensor_get_minutes(void);

    /**
     * @brief Fuerza una actualización manual de la temperatura.
     *        (solo si necesitas llamar desde otro módulo)
     */
    void sensor_force_update(void);

#ifdef __cplusplus
}
#endif
