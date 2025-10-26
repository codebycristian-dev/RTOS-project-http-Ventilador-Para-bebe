#include "adc.h"
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "ADC";

void adc_config(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTENUATION);
    adc1_config_channel_atten(ADC_CHANNEL_NTC, ADC_ATTENUATION);
    ESP_LOGI(TAG, "ADC configurado (potenciómetro en GPIO34, NTC en GPIO35)");
}

// Promedia lecturas del potenciómetro (0–255)
uint16_t adc_read_smooth(void)
{
    uint32_t lectura = 0;
    for (int i = 0; i < NUM_SAMPLES; i++)
        lectura += adc1_get_raw(ADC_CHANNEL);
    lectura /= NUM_SAMPLES;
    return (lectura * 255) / ADC_MAX_VALUE;
}

// Lee voltaje de la NTC
float adc_read_voltage_ntc(void)
{
    uint32_t raw = 0;
    for (int i = 0; i < NUM_SAMPLES; i++)
        raw += adc1_get_raw(ADC_CHANNEL_NTC);
    raw /= NUM_SAMPLES;
    return (raw * 3.3) / ADC_MAX_VALUE;
}
