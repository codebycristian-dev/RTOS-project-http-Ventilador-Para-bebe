#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "led_rgb.h"

/*
 * @brief
 * Configure the timer and channels for an RGB LED.
 */
void configurar_led(LED *led)
{
    /*Configure timer LEDC*/
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 4000, /*Frequency of 4 kHz*/
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer)); /*Check error when configuring timer*/

    /*Configure channel LEDC for red color*/
    ledc_channel_config_t ledc_channelR = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = led->CHANEL_R,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = led->PIN_R,
        .duty = led->Duty_R,
        .hpoint = 0};

    /*Configure channel LEDC for green color*/
    ledc_channel_config_t ledc_channelG = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = led->CHANEL_G,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = led->PIN_G,
        .duty = led->Duty_G,
        .hpoint = 0};

    /*Configure channel LEDC for blue color*/
    ledc_channel_config_t ledc_channelB = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = led->CHANEL_B,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = led->PIN_B,
        .duty = led->Duty_B,
        .hpoint = 0};

    /*Check errors when configuring channels*/
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channelR));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channelG));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channelB));
}

/*
 * @brief
 * Configure the timer and channel for a normal LED of a single color.
 */
void configurar_normal_led(NORMAL_LED *normal_led)
{
    /*Configure timer LEDC*/
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 4000, /*Frequency of 4 kHz*/
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer)); /*Check error when configuring timer*/

    /*Configure channel LEDC for normal LED*/
    ledc_channel_config_t NORMAL = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = normal_led->CHANNEL,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = normal_led->PIN,
        .duty = normal_led->DUTY,
        .hpoint = 0};

    /*Check error when configuring channel*/
    ESP_ERROR_CHECK(ledc_channel_config(&NORMAL));
}

/*
 * @brief
 * Initialize an RGB LED quickly with basic configuration.
 */
void initfastLED(void)
{
    /*Basic configuration for an RGB LED*/
    LED LED1 = {
        .CHANEL_B = LEDC_CHANNEL_0,
        .CHANEL_G = LEDC_CHANNEL_1,
        .CHANEL_R = LEDC_CHANNEL_2,
        .PIN_B = PINBB,
        .PIN_G = PINBG,
        .PIN_R = PINBR,
        .Duty_B = 0,
        .Duty_G = 0,
        .Duty_R = 0};

    /*Call the LED RGB configuration function*/
    configurar_led(&LED1);
}