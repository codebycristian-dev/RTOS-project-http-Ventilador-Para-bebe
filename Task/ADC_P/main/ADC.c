#include <stdio.h>
#include <math.h> // <---- necesario para log() cuadras NTC
#include "freertos/FreeRTOS.h" //libreria freertos 
#include "freertos/task.h" // libreria para tareas 
#include "esp_log.h" // libreria para anuncios por consola 
#include "esp_adc/adc_oneshot.h" // libreria ADC
#include "driver/ledc.h" // libreria PWM 
#include "driver/adc.h" // libreria ADC
#include "ledc.h" // Libreria configuración leds
#include "uart_user.h" // Libreria UART

//const static char *TAG = "EXAMPLE"; // tag para anuncios por consola

// -----------------------------------------------------------------------------
// Variables globales
// -----------------------------------------------------------------------------
#define NUM_SAMPLES 64 //Número de muestras para promediar y mermar ruido

LED led; //Estrucutra LED para creacion de led RGB
static adc_oneshot_unit_handle_t adc1_handle; // Implementación de manejador ADC1 

// -----------------------------------------------------------------------------
// Tarea: lee el potenciómetro (ADC6) y ajusta el LED verde
// -----------------------------------------------------------------------------
void myadc(void *pvParameters) // Tarea para leer ADC6 y medir voltaje y ajustar LED verde
{
    int raw = 0; // Variable para almacenar el valor crudo del ADC
    int temp = 0; // Variable temporal para lectura individual del ADC

    while (1)
    {
        raw = 0;
        // Lee 64 muestras y las pormedia para una lectura mas exacta 
        for (int i = 0; i < NUM_SAMPLES; i++) // ciclo hasta NUM_SAMPLES 
        {
            if (adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &temp) == ESP_OK)
            {
                raw += temp;
            }
        }
        raw /= NUM_SAMPLES;
        // Convierte el valor crudo a duty cycle, voltaje y porcentaje
        uint16_t duty = (raw * 255) / 4095;
        int voltage = (raw * 3300) / 4095;
        int percent = (duty * 100) / 255;

        // Actualizar duty del LED verde
        led.Duty_G = duty;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, led.CHANEL_G, led.Duty_G);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, led.CHANEL_G);

        ESP_LOGI("POTENCIOMETRO", "ADC6 Pot: Val ADC=%d  V=%dmV  Percent=%d%%", raw, voltage, percent);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// -----------------------------------------------------------------------------
// Tarea: lee el termistor NTC (ADC7) y ajusta el LED rojo
// -----------------------------------------------------------------------------
void myNTC(void *pvParameters)
{
    // --- Parámetros del sensor NTC ---
    const float BETA = 4100.0;  // Constante Beta del NTC
    const float T0 = 298.15;    // Temperatura de referencia (25°C en Kelvin)
    const float R0 = 10000.0;   // Resistencia del NTC a 25°C (10kΩ)
    const float Rfix = 10000.0; // Resistencia fija del divisor
    const float Vcc = 3.3;      // Voltaje de referencia ADC
    //const int NUM_SAMPLES = 64; // Promedio de muestras

    int raw_ntc = 0;
    int temp_adc = 0;

    while (1)
    {
        raw_ntc = 0;

        // Lee 64 muestras y las pormedia para una lectura mas exacta 
        for (int i = 0; i < NUM_SAMPLES; i++)
        {
            if (adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &temp_adc) == ESP_OK)
            {
                raw_ntc += temp_adc;
            }
        }
        raw_ntc /= NUM_SAMPLES;

        // --- Conversión a voltaje ---
        float Vout = (raw_ntc * Vcc) / 4095.0;

        // Evitar división por cero
        if (Vcc - Vout < 0.001)
            Vout = Vcc - 0.001;

        // --- Calcular resistencia del NTC ---
        float Rntc = (Rfix * Vout) / (Vcc - Vout);

        // --- Calcular temperatura (°C) usando ecuación Beta ---
        float T = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(Rntc / R0));
        float Tc = T - 273.15; // Conversión a grados Celsius

        // --- Escalar temperatura a brillo LED ---
        // 0°C -> duty = 0  |  60°C -> duty = 255
        int duty_red = (int)((Tc) * (255.0 / 60.0));

        // Limitar duty dentro de 0 a 255
        if (duty_red < 0)
            duty_red = 0;
        if (duty_red > 255)
            duty_red = 255;

        // --- Actualizar PWM del LED rojo ---
        led.Duty_R = duty_red;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, led.CHANEL_R, led.Duty_R);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, led.CHANEL_R);

        // --- Mostrar valores por consola ---
        ESP_LOGI("NTC", "ADC7=%d | Vout=%.3fV | Rntc=%.1fΩ | Temp=%.2f°C | Duty Red=%d",
                 raw_ntc, Vout, Rntc, Tc, duty_red);

        vTaskDelay(pdMS_TO_TICKS(1000)); // Esperar 1 segundo
    }
}

// -----------------------------------------------------------------------------
// Función principal
// -----------------------------------------------------------------------------
void app_main(void)
{
    // Inicializar ADC1
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // Configurar canales ADC6 (pot) y ADC7 (NTC)
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

    configurar_led(&led); // inicializa PWM LEDC

    // Crear tareas
    xTaskCreatePinnedToCore(myadc, "myadc", 4096, NULL, 5, NULL, 0); // Potenciómetro → LED verde
    xTaskCreatePinnedToCore(myNTC, "myNTC", 4096, NULL, 5, NULL, 1); // Termistor → LED rojo

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
