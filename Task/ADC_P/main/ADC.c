#include <stdio.h>
#include <math.h> // necesario para log() (NTC)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "ledc.h"
#include "uart_user.h"
#include "isr.h"

#define NUM_SAMPLES 64 // Número de muestras para promediar y reducir ruido

LED led;                                      // Estructura LED RGB
static adc_oneshot_unit_handle_t adc1_handle; // Manejador del ADC1
SemaphoreHandle_t xMutexADC;                  // Mutex para evitar lecturas simultáneas

// -----------------------------------------------------------------------------
// Tarea: Lee el potenciómetro (ADC6) y ajusta el LED verde
// -----------------------------------------------------------------------------
void myadc(void *pvParameters)
{
    int raw = 0;
    int temp = 0;

    while (1)
    {
        raw = 0;

        // Protección del ADC con Mutex
        if (xSemaphoreTake(xMutexADC, portMAX_DELAY))
        {
            for (int i = 0; i < NUM_SAMPLES; i++)
            {
                if (adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &temp) == ESP_OK)
                {
                    raw += temp;
                }
            }
            xSemaphoreGive(xMutexADC);
        }

        raw /= NUM_SAMPLES;

        uint16_t duty = (raw * 255) / 4095;
        int voltage = (raw * 3300) / 4095;
        int percent = (duty * 100) / 255;

        // Actualiza el duty del LED verde
        led.Duty_G = duty;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, led.CHANEL_G, led.Duty_G);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, led.CHANEL_G);

        ESP_LOGI("ADC", "ADC6 Pot: Val=%d V=%dmV Percent=%d%%", raw, voltage, percent);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// -----------------------------------------------------------------------------
// Tarea: Lee el NTC (ADC7) y ajusta el LED rojo según la temperatura
// -----------------------------------------------------------------------------
void myNTC(void *pvParameters)
{
    const float BETA = 4100.0;  // Constante Beta del NTC
    const float T0 = 298.15;    // Temperatura de referencia (25°C)
    const float R0 = 10000.0;   // Resistencia del NTC a 25°C
    const float Rfix = 10000.0; // Resistencia fija del divisor
    const float Vcc = 3.3;      // Voltaje de referencia ADC

    int raw_ntc = 0;
    int temp_adc = 0;

    while (1)
    {
        raw_ntc = 0;

        // Protección del ADC con Mutex
        if (xSemaphoreTake(xMutexADC, portMAX_DELAY))
        {
            for (int i = 0; i < NUM_SAMPLES; i++)
            {
                if (adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &temp_adc) == ESP_OK)
                {
                    raw_ntc += temp_adc;
                }
            }
            xSemaphoreGive(xMutexADC);
        }

        raw_ntc /= NUM_SAMPLES;

        // --- Conversión a voltaje ---
        float Vout = (raw_ntc * Vcc) / 4095.0;
        if (Vcc - Vout < 0.001)
            Vout = Vcc - 0.001;

        // --- Calcular resistencia del NTC ---
        float Rntc = (Rfix * Vout) / (Vcc - Vout);

        // --- Calcular temperatura en °C ---
        float T = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(Rntc / R0));
        float Tc = T - 273.15;

        // --- Escalar temperatura a duty (0-60°C → 0-255) ---
        int duty_red = (int)(Tc * (255.0 / 60.0));
        if (duty_red < 0)
            duty_red = 0;
        if (duty_red > 255)
            duty_red = 255;

        led.Duty_R = duty_red;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, led.CHANEL_R, led.Duty_R);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, led.CHANEL_R);

        ESP_LOGI("NTC", "ADC7=%d Vout=%.3fV Rntc=%.1fΩ Temp=%.2f°C Duty=%d",
                 raw_ntc, Vout, Rntc, Tc, duty_red);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// -----------------------------------------------------------------------------
// Función principal
// -----------------------------------------------------------------------------
void app_main(void)
{
    init_uart();
    isr_install();

    // Crear mutex para proteger acceso al ADC
    xMutexADC = xSemaphoreCreateMutex();
    if (xMutexADC == NULL)
    {
        ESP_LOGE("MAIN", "Error: No se pudo crear el mutex ADC");
        return;
    }

    // Inicializar ADC1
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // Configurar canales ADC6 (potenciómetro) y ADC7 (NTC)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_WIDTH_BIT_12,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &config));

    // Configurar LED RGB
    led.CHANEL_G = LEDC_CHANNEL_0;
    led.PIN_G = 18;
    led.Duty_G = 0;

    led.CHANEL_R = LEDC_CHANNEL_1;
    led.PIN_R = 5;
    led.Duty_R = 0;

    led.CHANEL_B = LEDC_CHANNEL_2;
    led.PIN_B = 19;
    led.Duty_B = 0;

    configurar_led(&led); // inicializa PWM LEDC

    // Crear tareas
    xTaskCreatePinnedToCore(myadc, "myadc", 4096, NULL, 5, NULL, 0); // Potenciómetro → LED verde
    xTaskCreatePinnedToCore(myNTC, "myNTC", 4096, NULL, 5, NULL, 1); // Termistor → LED rojo

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}