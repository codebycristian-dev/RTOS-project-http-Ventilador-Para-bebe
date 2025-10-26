#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "LED_RGB.h"
#include "uart.h"
#include "adc.h"
#include "isr.h"
#include <math.h>
#include "esp_log.h"

// Global variables
QueueHandle_t queue;
LED LED1;
LED LED2;

// Function prototypes
void task_range(void *args);
void update_led(ledc_channel_t channel, const char *channel_name, int duty);
void update_duty(LED* led, uint16_t flags, uint16_t duty);
void task_adc(void *args);
void led_init(void);
void process_command(char *command);
void verify_entry(void);

static const char *TAG = "Main";

/*Transform voltage of NTC to Temperature
@param voltage value of voltage to in ntc */
float voltage_to_temp(float voltage){

    // Calcular la resistencia de la NTC
    float resistencia_ntc = (22 * voltage) / (3.3 - voltage);

    // Aplicar la ecuación de Steinhart-Hart
    float inv_T = 31000/(log(resistencia_ntc/10000) + 31000/298.15);

    // Convertir de Kelvin a Celsius
    float temperatura = inv_T - 273.15;

    return temperatura-27;

}

// Task to handle range-based LED control
void task_range(void *args) {
    uint16_t temp_minG = 0;
    uint16_t temp_maxG = 0;
    uint16_t temp_minB = 0;
    uint16_t temp_maxB = 0;
    uint16_t temp_minr = 0;
    uint16_t temp_maxr = 0;
    char message[20];
    while (1) {
        if (xQueuePeek(queue, message,  pdMS_TO_TICKS(10))) {


            ESP_LOGI(TAG, "Receiver: Recibido %s", message);
            char *token1 = strtok(message, " ");
            char *token2 = strtok(NULL, " ");
            char *token3 = strtok(NULL, " ");

            if (token1 != NULL && token2 != NULL && token3 != NULL) {
                if (strcmp(token2, "R") == 0) {
                    if (strcmp(token1, "m") == 0) {
                        temp_minr = atoi(token3);
                    } else if (strcmp(token1, "x") == 0) {
                        temp_maxr = atoi(token3);
                    }
                } else if (strcmp(token2, "G") == 0) {
                    if (strcmp(token1, "m") == 0) {
                        temp_minG = atoi(token3);
                    } else if (strcmp(token1, "x") == 0) {
                        temp_maxG = atoi(token3);
                    }
                } else if (strcmp(token2, "B") == 0) {
                    if (strcmp(token1, "m") == 0) {
                        temp_minB = atoi(token3);
                    } else if (strcmp(token1, "x") == 0) {
                        temp_maxB = atoi(token3);
                    }
                }
            } else {
                ESP_LOGE(TAG, "Error: Invalid message format");
            }
            xQueueReset(queue);
         }
        uint16_t duty_value = adc1_get_raw(ADC_CHANNEL_2);
        if (duty_value == 0xFFFF) {
            ESP_LOGE(TAG, "Error reading ADC");
            continue;
        }

        float voltage = (duty_value * 3.3) / 4095;
        float temperature = voltage_to_temp(voltage);

        ESP_LOGI(TAG, "Temperature: %.2f", temperature);

        if (temperature >= temp_minr && temperature <= temp_maxr) {
            update_led(LED2.CHANEL_R, "RED", 150);
        } else {
            update_led(LED2.CHANEL_R, "RED", 0);
        }
        if (temperature >= temp_minG && temperature <= temp_maxG) {
            update_led(LED2.CHANEL_G, "GREEN", 150);
        } else {
            update_led(LED2.CHANEL_G, "GREEN", 0);
        }
        if (temperature >= temp_minB && temperature <= temp_maxB) {
            update_led(LED2.CHANEL_B, "BLUE", 150);
        } else {
            update_led(LED2.CHANEL_B, "BLUE", 0);
        }

            vTaskDelay(pdMS_TO_TICKS(600));
        }

        
}

// Function to update LED duty cycle
void update_led(ledc_channel_t channel, const char *channel_name, int duty) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
    //ESP_LOGI(TAG, "UPDATE %s TO: %d", channel_name, duty);
}

// Function to update duty cycle for a specific LED
void update_duty(LED* led, uint16_t flags, uint16_t duty) {
    switch (flags) {
        case 0:
            led->Duty_R = duty;
            update_led(led->CHANEL_R, "RED", led->Duty_R);
            break;
        case 1:
            led->Duty_G = duty;
            update_led(led->CHANEL_G, "GREEN", led->Duty_G);
            break;
        case 2:
            led->Duty_B = duty;
            update_led(led->CHANEL_B, "BLUE", led->Duty_B);
            break;
        case 3:
            ESP_LOGI(TAG, "No LED selected");
            break;
    }
}

// Task to handle ADC readings
void task_adc(void *args) {
    
    while (1) {
        uint16_t duty_value = adc_read_smooth();  
        update_duty(&LED1, flags, duty_value);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Function to initialize LEDs
void led_init(void) {
    LED1 = (LED){
        .CHANEL_R = LEDC_CHANNEL_0, .CHANEL_G = LEDC_CHANNEL_1, .CHANEL_B = LEDC_CHANNEL_2,
        .PIN_R = 5, .PIN_G = 18, .PIN_B = 19,
        .Duty_R = 0, .Duty_G = 0, .Duty_B = 0
    };
    LED2 = (LED){
        .CHANEL_R = LEDC_CHANNEL_3, .CHANEL_G = LEDC_CHANNEL_4, .CHANEL_B = LEDC_CHANNEL_5,
        .PIN_R = 14, .PIN_G = 13, .PIN_B = 12,
        .Duty_R = 0, .Duty_G = 0, .Duty_B = 0
    };
    configurar_led(&LED1);
    configurar_led(&LED2);
    ESP_LOGI(TAG, "LEDs initialized successfully");
}

void process_command(char *command) {
    char *token1 = strtok(command, " ");
    if (token1 == NULL || (strcmp(token1, "m") != 0 && strcmp(token1, "x") != 0)) {
        return;
    }

    char *token2 = strtok(NULL, " ");
    if (token2 == NULL || (strcmp(token2, "R") != 0 && strcmp(token2, "G") != 0 && strcmp(token2, "B") != 0)) {
        ESP_LOGE(TAG, "Error: Second word is not 'R', 'G', or 'B'.");
        return;
    }

    char *token3 = strtok(NULL, " ");
    if (token3 == NULL) {
        ESP_LOGE(TAG, "Error: No third word.");
        return;
    }

    int value = atoi(token3);
    if (value < 0 || value > 150) {
        ESP_LOGE(TAG, "Error: Value must be between 0 and 150.");
        return;
    }

    ESP_LOGI(TAG, "Valid command: %s %s %d", token1, token2, value);

    char *message = malloc(20);
    if (message == NULL) {
        ESP_LOGE(TAG, "Error: No se pudo asignar memoria");
        return;
    }
    snprintf(message, 20, "%s %s %d\n", token1, token2, value);
    if (xQueueSend(queue, message, pdMS_TO_TICKS(100)) == pdPASS) {
        ESP_LOGI(TAG, "Sender: Enviado '%s'", message);
    } else {
        ESP_LOGE(TAG, "Sender: Error al enviar '%s'", message);
        free(message);
    }
}

void verify_entry(void) {
    char *data = readuart();
    if (data != NULL) {
        ESP_LOGI(TAG, "Data received from UART: %s", data);
        process_command(data);
        free(data);
    } else {
        ESP_LOGI(TAG, "No data received from UART");
    }
    data = NULL;
}
// Main application function
void app_main(void) {
    isr_install();
    adc_config();
    adc_config2();
    led_init();
    init_uart0();

    queue = xQueueCreate(10, sizeof(char[20]));
    if (queue == NULL) {
        ESP_LOGE(TAG, "Error creating queue");
        return;
    }

    esp_err_t ret = xTaskCreatePinnedToCore(task_adc, "task_adc", 4096, NULL, 2, NULL, 0);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Error creating ADC task: %d", ret);
        return;
    }

    esp_err_t ret2 = xTaskCreatePinnedToCore(task_range, "task_range", 4096, NULL, 3, NULL, 1);
    if (ret2 != pdPASS) {
        ESP_LOGE(TAG, "Error creating range task: %d", ret2);
        return;
    }

    while (1) {
        verify_entry();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}