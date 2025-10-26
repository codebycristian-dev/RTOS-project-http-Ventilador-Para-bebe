#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "led_rgb.h"

/*
 * @brief
 * Configura el temporizador y los canales LEDC para un LED RGB.
 */
void configurar_led(LED* led) {
    // Configuración del temporizador LEDC
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .freq_hz          = 4000,  // Frecuencia de 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));  // Comprobar error al configurar temporizador

    // Configuración del canal LEDC para el color rojo
    ledc_channel_config_t ledc_channelR = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = led->CHANEL_R,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = led->PIN_R,
        .duty           = led->Duty_R,
        .hpoint         = 0
    };

    // Configuración del canal LEDC para el color verde
    ledc_channel_config_t ledc_channelG = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = led->CHANEL_G,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = led->PIN_G,
        .duty           = led->Duty_G,
        .hpoint         = 0
    };

    // Configuración del canal LEDC para el color azul
    ledc_channel_config_t ledc_channelB = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = led->CHANEL_B,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = led->PIN_B,
        .duty           = led->Duty_B,
        .hpoint         = 0
    };

    // Comprobar errores al configurar los canales
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channelR));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channelG));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channelB));
}

/*
 * @brief
 * Configura el temporizador y el canal LEDC para un LED normal de un solo color.
 */
void configurar_normal_led(NORMAL_LED* normal_led) {
    // Configuración del temporizador LEDC
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .freq_hz          = 4000,  // Frecuencia de 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));  // Comprobar error al configurar temporizador

    // Configuración del canal LEDC para el LED normal
    ledc_channel_config_t NORMAL = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = normal_led->CHANNEL,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = normal_led->PIN,
        .duty           = normal_led->DUTY,
        .hpoint         = 0
    };

    // Comprobar error al configurar el canal
    ESP_ERROR_CHECK(ledc_channel_config(&NORMAL));
}

/*
 * @brief
 * Inicializa un LED RGB rápidamente con una configuración básica.
 */
void initfastLED(void) {
    // Configuración inicial básica para un LED RGB
    LED LED1 = {
        .CHANEL_B = LEDC_CHANNEL_0,
        .CHANEL_G = LEDC_CHANNEL_1,
        .CHANEL_R = LEDC_CHANNEL_2,
        .PIN_B = PINBB,
        .PIN_G = PINBG,
        .PIN_R = PINBR,
        .Duty_B = 0,
        .Duty_G = 0,
        .Duty_R = 0
    };

    // Llamar a la función de configuración del LED RGB
    configurar_led(&LED1);
}
