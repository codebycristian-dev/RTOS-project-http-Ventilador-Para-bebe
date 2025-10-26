#include "ledc.h"
#include "esp_log.h"

static const char *TAG = "LEDC";

void configurar_led(LED *led)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 4000,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&timer);

    ledc_channel_config_t channels[3] = {
        {.channel = led->CH_R, .gpio_num = PIN_R, .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = LEDC_TIMER_0, .duty = 0},
        {.channel = led->CH_G, .gpio_num = PIN_G, .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = LEDC_TIMER_0, .duty = 0},
        {.channel = led->CH_B, .gpio_num = PIN_B, .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = LEDC_TIMER_0, .duty = 0}};

    for (int i = 0; i < 3; i++)
        ledc_channel_config(&channels[i]);

    ESP_LOGI(TAG, "LED RGB configurado en GPIO25,26,27");
}

void update_led_duty(ledc_channel_t channel, uint16_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}
