#include "adc.h"
#include <stdio.h>

void adc_config(void) {
    esp_err_t ret = adc1_config_width(ADC_WIDTH_BIT_12);
    if (ret != ESP_OK) {
        printf("Error configuring ADC width: %d\n", ret);
        return;
    }

    ret = adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTENUATION);
    if (ret != ESP_OK) {
        printf("Error configuring ADC attenuation: %d\n", ret);
        return;
    }

    printf("ADC configured with channel %d and attenuation %d dB\n", ADC_CHANNEL, ADC_ATTENUATION);
}

uint16_t adc_read_smooth(void) {
    int smooth_reading = 0;

    // Read ADC multiple times and calculate the average
    for (uint16_t i = 0; i < NUM_SAMPLES; i++) {
        smooth_reading += adc1_get_raw(ADC_CHANNEL);
    }
    smooth_reading /= NUM_SAMPLES;

    // Scale the ADC value to the range 0-255
    return (smooth_reading * 255) / ADC_MAX_VALUE;
}


void adc_config2(void) {
    esp_err_t ret = adc1_config_width(ADC_WIDTH_BIT_12);
    if (ret != ESP_OK) {
        printf("Error configuring ADC width: %d\n", ret);
        return;
    }

    ret = adc1_config_channel_atten(ADC_CHANNEL_2, ADC_ATTENUATION);
    if (ret != ESP_OK) {
        printf("Error configuring ADC attenuation: %d\n", ret);
        return;
    }

    printf("ADC configured with channel %d and attenuation %d dB\n", ADC_CHANNEL_2, ADC_ATTENUATION);
}

uint16_t adc_read_smooth2(void) {
    int smooth_reading2 = 0;

    // Read ADC multiple times and calculate the average
    for (uint16_t i = 0; i < NUM_SAMPLES; i++) {
        smooth_reading2 += adc1_get_raw(ADC_CHANNEL_2);
    }
    smooth_reading2 /= NUM_SAMPLES;

    // Scale the ADC value to the range 0-255
    return (smooth_reading2 * 255) / ADC_MAX_VALUE;
}

