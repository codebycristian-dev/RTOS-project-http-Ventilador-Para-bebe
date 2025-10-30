#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/ledc.h"
#include "esp_err.h"
#include "ledc.h"

// =================== Helpers internos ===================

static inline uint32_t apply_invert_if_needed(const LED *led, uint32_t duty255)
{
    // Para ánodo común se invierte la lógica (0->255, 255->0)
    return led->common_anode ? (255u - duty255) : duty255;
}

static void ledc_config_timer(ledc_mode_t mode, ledc_timer_t tmr, ledc_timer_bit_t res, uint32_t freq_hz)
{
    ledc_timer_config_t tcfg = {
        .speed_mode = mode,
        .timer_num = tmr,
        .duty_resolution = res,
        .freq_hz = freq_hz,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&tcfg));
}

static void ledc_config_channel(ledc_mode_t mode, ledc_channel_t ch, ledc_timer_t tmr, int gpio, uint32_t duty_init)
{
    ledc_channel_config_t ccfg = {
        .speed_mode = mode,
        .channel = ch,
        .timer_sel = tmr,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = gpio,
        .duty = duty_init,
        .hpoint = 0,
#if ESP_IDF_VERSION_MAJOR >= 5
        .flags.output_invert = 0,
#endif
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ccfg));
}

// =================== API pública ===================

void configurar_led(LED *led)
{
    // Defaults sensatos si no se asignaron desde la app
    if (led->SPEED_MODE != LEDC_LOW_SPEED_MODE && led->SPEED_MODE != LEDC_HIGH_SPEED_MODE)
        led->SPEED_MODE = LEDC_LOW_SPEED_MODE;

    if (led->RES == 0)
        led->RES = LEDC_TIMER_8_BIT; // 8 bits
    if (led->FREQ_HZ == 0)
        led->FREQ_HZ = 2000; // 2 kHz

    // Por robustez: 3 timers (uno por color)
    if (led->TIMER_R < LEDC_TIMER_0 || led->TIMER_R > LEDC_TIMER_3)
        led->TIMER_R = LEDC_TIMER_0;
    if (led->TIMER_G < LEDC_TIMER_0 || led->TIMER_G > LEDC_TIMER_3)
        led->TIMER_G = LEDC_TIMER_1;
    if (led->TIMER_B < LEDC_TIMER_0 || led->TIMER_B > LEDC_TIMER_3)
        led->TIMER_B = LEDC_TIMER_2;

    // Timers
    ledc_config_timer(led->SPEED_MODE, led->TIMER_R, led->RES, led->FREQ_HZ);
    ledc_config_timer(led->SPEED_MODE, led->TIMER_G, led->RES, led->FREQ_HZ);
    ledc_config_timer(led->SPEED_MODE, led->TIMER_B, led->RES, led->FREQ_HZ);

    // Canales RGB
    ledc_config_channel(led->SPEED_MODE, led->CHANEL_R, led->TIMER_R, led->PIN_R, 0);
    ledc_config_channel(led->SPEED_MODE, led->CHANEL_G, led->TIMER_G, led->PIN_G, 0);
    ledc_config_channel(led->SPEED_MODE, led->CHANEL_B, led->TIMER_B, led->PIN_B, 0);

    // Arranque: negro
    ledc_set_duty(led->SPEED_MODE, led->CHANEL_R, 0);
    ledc_update_duty(led->SPEED_MODE, led->CHANEL_R);
    ledc_set_duty(led->SPEED_MODE, led->CHANEL_G, 0);
    ledc_update_duty(led->SPEED_MODE, led->CHANEL_G);
    ledc_set_duty(led->SPEED_MODE, led->CHANEL_B, 0);
    ledc_update_duty(led->SPEED_MODE, led->CHANEL_B);

    led->Duty_R = led->Duty_G = led->Duty_B = 0;
}

void led_rgb_write(LED *led, uint8_t r, uint8_t g, uint8_t b)
{
    // Aplica inversión si es ánodo común
    uint32_t dr = apply_invert_if_needed(led, r);
    uint32_t dg = apply_invert_if_needed(led, g);
    uint32_t db = apply_invert_if_needed(led, b);

    // Carga los tres duty
    ledc_set_duty(led->SPEED_MODE, led->CHANEL_R, dr);
    ledc_set_duty(led->SPEED_MODE, led->CHANEL_G, dg);
    ledc_set_duty(led->SPEED_MODE, led->CHANEL_B, db);

    // Actualiza sincronizado
    ledc_update_duty(led->SPEED_MODE, led->CHANEL_R);
    ledc_update_duty(led->SPEED_MODE, led->CHANEL_G);
    ledc_update_duty(led->SPEED_MODE, led->CHANEL_B);

    // Guarda estado (no afecta PWM)
    led->Duty_R = r;
    led->Duty_G = g;
    led->Duty_B = b;
}

void configurar_normal_led(NORMAL_LED *normal_led)
{
    // Timer único para el canal simple
    ledc_timer_config_t tcfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_3,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 2000,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&tcfg));

    ledc_channel_config_t ccfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = normal_led->CHANNEL,
        .timer_sel = LEDC_TIMER_3,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = normal_led->PIN,
        .duty = normal_led->DUTY,
        .hpoint = 0,
#if ESP_IDF_VERSION_MAJOR >= 5
        .flags.output_invert = 0,
#endif
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ccfg));

    // Arranque en 0
    ledc_set_duty(LEDC_LOW_SPEED_MODE, normal_led->CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, normal_led->CHANNEL);
}

void initfastLED(void)
{
    // Inicialización rápida con defaults sensatos
    LED fast = {
        .CHANEL_R = LEDC_CHANNEL_0,
        .CHANEL_G = LEDC_CHANNEL_1,
        .CHANEL_B = LEDC_CHANNEL_2,
        .PIN_R = PINBR,
        .PIN_G = PINBG,
        .PIN_B = PINBB,
        .Duty_R = 0,
        .Duty_G = 0,
        .Duty_B = 0,
        .RES = LEDC_TIMER_8_BIT,
        .SPEED_MODE = LEDC_LOW_SPEED_MODE,
        .TIMER_R = LEDC_TIMER_0,
        .TIMER_G = LEDC_TIMER_1,
        .TIMER_B = LEDC_TIMER_2,
        .FREQ_HZ = 2000,
        .common_anode = false};

    configurar_led(&fast);

    // Ejemplo: encender blanco tenue 1s y apagar
    led_rgb_write(&fast, 64, 64, 64);
    vTaskDelay(pdMS_TO_TICKS(1000));
    led_rgb_write(&fast, 0, 0, 0);
}
