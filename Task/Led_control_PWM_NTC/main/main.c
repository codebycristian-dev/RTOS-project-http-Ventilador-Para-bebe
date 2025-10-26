#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "adc.h"
#include "ledc.h"
#include "isr.h"
#include "uart.h"

// --- Variables globales ---
static const char *TAG = "MAIN";
LED led1;
float thrR_min = 0, thrR_max = 15;
float thrG_min = 10, thrG_max = 30;
float thrB_min = 40, thrB_max = 50;

// --- Funciones ---
float voltage_to_temp(float voltage)
{
    const float Vref = 3.3;     // voltaje de referencia ADC
    const float Rref = 22000.0; // resistencia fija (22 kΩ)
    const float R0 = 10000.0;   // resistencia del NTC a 25 °C
    const float B = 4100.0;     // constante Beta de tu NTC
    const float T0 = 298.15;    // temperatura de referencia (25 °C en Kelvin)

    // Calcular resistencia del NTC (NTC hacia 3.3V, Rref hacia GND)
    float Rntc = (Rref * voltage) / (Vref - voltage);

    // Aplicar ecuación Beta
    float inv_T = (1.0 / T0) + (1.0 / B) * log(Rntc / R0);
    float tempK = 1.0 / inv_T;

    return tempK - 273.15; // convertir a °C
}

void process_uart_command(char *cmd)
{
    char color;
    float min, max;
    if (sscanf(cmd, "%c %f %f", &color, &min, &max) == 3)
    {
        switch (color)
        {
        case 'R':
            thrR_min = min;
            thrR_max = max;
            break;
        case 'G':
            thrG_min = min;
            thrG_max = max;
            break;
        case 'B':
            thrB_min = min;
            thrB_max = max;
            break;
        }
        ESP_LOGI(TAG, "Nuevo threshold %c: %.1f - %.1f", color, min, max);
    }
}

void app_main(void)
{
    init_uart0();
    adc_config();
    isr_install();

    // LED de estado del botón
    gpio_reset_pin(GPIO_NUM_23);
    gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT);

    // Configurar LED RGB
    led1.CH_R = LEDC_CHANNEL_0;
    led1.CH_G = LEDC_CHANNEL_1;
    led1.CH_B = LEDC_CHANNEL_2;
    configurar_led(&led1);

    ESP_LOGI(TAG, "Sistema iniciado");

    while (1)
    {
        // Leer comandos UART
        char *cmd = read_uart();
        if (cmd != NULL)
            process_uart_command(cmd);

        // Leer temperatura
        float voltage = adc_read_voltage_ntc();
        float temp = voltage_to_temp(voltage);

        // Actualizar color segun thresholds
        uint16_t intensity = adc_read_smooth();
        uint8_t R = 0, G = 0, B = 0;

        if (temp >= thrR_min && temp <= thrR_max)
            R = intensity;
        else if (temp >= thrG_min && temp <= thrG_max)
            G = intensity;
        else if (temp >= thrB_min && temp <= thrB_max)
            B = intensity;

        update_led_duty(led1.CH_R, R);
        update_led_duty(led1.CH_G, G);
        update_led_duty(led1.CH_B, B);

        // Mostrar temperatura solo si está habilitado
        if (print_enabled)
        {
            gpio_set_level(GPIO_NUM_23, 1); // Activa LED/motor indicador
            ESP_LOGI(TAG, "Temp: %.2f °C  (R:%d G:%d B:%d)", temp, R, G, B);
        }
        else
        {
            gpio_set_level(GPIO_NUM_23, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
