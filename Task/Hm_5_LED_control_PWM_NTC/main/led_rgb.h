#ifndef LEDCUSER_H
#define LEDCUSER_H

#include "driver/ledc.h"
#include "esp_err.h"

// Definir los pines del LED RGB
#define PINBR           5
#define PINBG           18
#define PINBB           19
#define PINBUZZER       21
#define CHBUZZER        LEDC_CHANNEL_3

// Definir la estructura para LED RGB
typedef struct {
    ledc_channel_t CHANEL_R; // Canal para el color rojo
    ledc_channel_t CHANEL_G; // Canal para el color verde
    ledc_channel_t CHANEL_B; // Canal para el color azul

    uint16_t PIN_R; // Pin GPIO para PWM del color rojo
    uint16_t PIN_G; // Pin GPIO para PWM del color verde
    uint16_t PIN_B; // Pin GPIO para PWM del color azul

    uint16_t Duty_R; // Valor del duty cycle para el color rojo
    uint16_t Duty_G; // Valor del duty cycle para el color verde
    uint16_t Duty_B; // Valor del duty cycle para el color azul
} LED;

// Definir la estructura para un LED normal
typedef struct {
    ledc_channel_t CHANNEL; // Canal para PWM
    uint16_t PIN;           // Pin GPIO para salida
    uint16_t DUTY;          // Duty cycle para la salida PWM
} NORMAL_LED;

extern LED LED1;
extern NORMAL_LED Buzzer;

// Declaración de funciones
void configurar_led(LED* led);
void configurar_normal_led(NORMAL_LED* normal_led);
void initfastLED(void);

#endif // LEDCUSER_H