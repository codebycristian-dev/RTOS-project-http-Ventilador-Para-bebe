#ifndef LEDCUSER_H
#define LEDCUSER_H

#include "driver/ledc.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

// ==========================================================
// Pines por defecto (puedes cambiarlos si deseas)
// ==========================================================
#define PINBR 5  // Rojo
#define PINBG 18 // Verde
#define PINBB 19 // Azul

#define PINBUZZER 21
#define CHBUZZER LEDC_CHANNEL_3

// ==========================================================
// Estructura LED RGB
// ==========================================================
typedef struct
{
    // --- Canales PWM ---
    ledc_channel_t CHANEL_R;
    ledc_channel_t CHANEL_G;
    ledc_channel_t CHANEL_B;

    // --- Pines GPIO ---
    uint16_t PIN_R;
    uint16_t PIN_G;
    uint16_t PIN_B;

    // --- Valores de duty (0-255) ---
    uint16_t Duty_R;
    uint16_t Duty_G;
    uint16_t Duty_B;

    // --- Configuración general ---
    ledc_timer_bit_t RES;   // Resolución (LEDC_TIMER_8_BIT, etc.)
    ledc_mode_t SPEED_MODE; // LEDC_LOW_SPEED_MODE o HIGH
    ledc_timer_t TIMER_R;   // Timer usado por cada canal
    ledc_timer_t TIMER_G;
    ledc_timer_t TIMER_B;
    uint32_t FREQ_HZ;  // Frecuencia PWM
    bool common_anode; // true = ánodo común (inversión de duty)
} LED;

// ==========================================================
// Estructura LED normal (1 canal)
// ==========================================================
typedef struct
{
    ledc_channel_t CHANNEL; // Canal PWM
    uint16_t PIN;           // Pin GPIO
    uint16_t DUTY;          // Duty (0–255)
} NORMAL_LED;

// ==========================================================
// Prototipos de funciones públicas
// ==========================================================
void configurar_led(LED *led);                                 // Configura LED RGB completo
void led_rgb_write(LED *led, uint8_t r, uint8_t g, uint8_t b); // Actualiza colores

void configurar_normal_led(NORMAL_LED *normal_led);
void initfastLED(void); // Inicialización rápida por defecto

#endif // LEDCUSER_H
