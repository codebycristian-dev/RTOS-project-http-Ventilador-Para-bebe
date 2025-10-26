#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"

// Configuración de canales ADC (ESP32 DevKit V1)
#define ADC_CHANNEL ADC1_CHANNEL_6     // GPIO34 → potenciómetro
#define ADC_CHANNEL_NTC ADC1_CHANNEL_7 // GPIO35 → sensor NTC

#define ADC_ATTENUATION ADC_ATTEN_DB_11
#define NUM_SAMPLES 64
#define ADC_MAX_VALUE 4095

// Funciones
void adc_config(void);
uint16_t adc_read_smooth(void);
float adc_read_voltage_ntc(void);

#endif