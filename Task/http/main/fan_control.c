#include "fan_control.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "fan_control";
/**
 * @brief Inicializa el control del ventilador mediante PWM.
 */
// PWM parameters
#define FAN_PWM_GPIO 14    // GPIO asignado
#define FAN_PWM_FREQ 25000 // 25 kHz (ideal para ventilador)
#define FAN_PWM_RES LEDC_TIMER_8_BIT
#define FAN_PWM_TIMER LEDC_TIMER_0
#define FAN_PWM_CHANNEL LEDC_CHANNEL_0

static int current_pwm_percent = 0; // PWM actual en %
/**
 * @brief Inicializa el control del ventilador mediante PWM.
 */
void fan_control_init(void)
{
    ESP_LOGI(TAG, "Inicializando ventilador PWM GPIO %d...", FAN_PWM_GPIO);

    // Configurar el timer LEDC
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = FAN_PWM_TIMER,
        .duty_resolution = FAN_PWM_RES,
        .freq_hz = FAN_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&timer_conf);

    // Configurar el canal LEDC
    ledc_channel_config_t channel_conf = {
        .gpio_num = FAN_PWM_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = FAN_PWM_CHANNEL,
        .timer_sel = FAN_PWM_TIMER,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&channel_conf);

    ESP_LOGI(TAG, "Ventilador PWM inicializado.");
}
/**
 * @brief Establece el valor de PWM del ventilador en porcentaje (0-100%).
 * @param percent Porcentaje de PWM (0-100).
 */
void fan_set_pwm(int percent)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;

    current_pwm_percent = percent;

    int max_duty = (1 << FAN_PWM_RES) - 1; // Para 8 bits = 255
    int duty = (percent * max_duty) / 100;

    ESP_LOGI(TAG, "PWM Ventilador = %d%% (duty=%d)", percent, duty);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, FAN_PWM_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, FAN_PWM_CHANNEL);
}
/**
 * @brief Obtiene el valor actual de PWM del ventilador en porcentaje (0-100%).
 * @return Porcentaje de PWM actual (0-100).
 */
int fan_get_current_pwm(void)
{
    return current_pwm_percent;
}
