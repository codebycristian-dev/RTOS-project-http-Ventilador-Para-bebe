#ifndef LEDCUSER_H
#define LEDCUSER_H

#include "driver/ledc.h"
#include "esp_err.h"
#include <stdint.h>

// Pines del LED RGB
#define PIN_R 25
#define PIN_G 26
#define PIN_B 27

typedef struct
{
    ledc_channel_t CH_R;
    ledc_channel_t CH_G;
    ledc_channel_t CH_B;
    uint16_t Duty_R;
    uint16_t Duty_G;
    uint16_t Duty_B;
} LED;

// Funciones
void configurar_led(LED *led);
void update_led_duty(ledc_channel_t channel, uint16_t duty);

#endif
