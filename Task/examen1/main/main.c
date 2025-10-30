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

/* Global variables */
QueueHandle_t queue;
LED LED1;
LED LED2;
uint16_t temp_on = 1;

/* Function prototypes
@brief
    Task to handle range-based LED control
    Update LEDs based on temperature ranges

    @param args
        Pointer to the task arguments
*/

void task_range(void *args);
void update_led(ledc_channel_t channel, const char *channel_name, int duty);
void update_duty(LED *led, uint16_t flags, uint16_t duty);
void task_adc(void *args);
void led_init(void);
void process_command(char *command);
void verify_entry(void);

static const char *TAG = "Main";

/* Transform voltage of NTC to Temperature
@param voltage value of voltage in ntc */

float voltage_to_temp(float voltage)
{
    // Calculate the resistance of the NTC
    float resistencia_ntc = (22 * voltage) / (3.3 - voltage);

    // Apply the Steinhart-Hart equation
    float inv_T = 31000 / (log(resistencia_ntc / 10000) + 31000 / 298.15);

    // Convert from Kelvin to Celsius
    float temperatura = inv_T - 273.15;

    return temperatura - 27;
}

/* Task to handle range-based LED control
@brief
Update LEDs based on temperature ranges

@param args
Pointer to the task arguments
*/

void task_range(void *args)
{
    uint16_t temp_minG = 0;
    uint16_t temp_maxG = 0;
    uint16_t temp_minB = 0;
    uint16_t temp_maxB = 0;
    uint16_t temp_minr = 0;
    uint16_t temp_maxr = 0;

    char message[20];
    while (1)
    {
        /* Receive message from UART */
        if (xQueueReceive(queue, message, pdMS_TO_TICKS(10)) == pdPASS)
        {
            ESP_LOGI(TAG, "Receiver: Received %s", message);

            char *token1 = strtok(message, " ");
            char *token2 = strtok(NULL, " ");
            char *token3 = strtok(NULL, " ");

            /* Check if message is valid */

            if (token1 != NULL && token2 != NULL && token3 != NULL)
            {
                if (strcmp(token2, "R") == 0)
                {
                    if (strcmp(token1, "m") == 0)
                    {
                        temp_minr = atoi(token3);
                    }
                    else if (strcmp(token1, "x") == 0)
                    {
                        temp_maxr = atoi(token3);
                    }
                }
                else if (strcmp(token2, "G") == 0)
                {
                    if (strcmp(token1, "m") == 0)
                    {
                        temp_minG = atoi(token3);
                    }
                    else if (strcmp(token1, "x") == 0)
                    {
                        temp_maxG = atoi(token3);
                    }
                }
                else if (strcmp(token2, "B") == 0)
                {
                    if (strcmp(token1, "m") == 0)
                    {
                        temp_minB = atoi(token3);
                    }
                    else if (strcmp(token1, "x") == 0)
                    {
                        temp_maxB = atoi(token3);
                    }
                }
            }
            else
            {
                ESP_LOGE(TAG, "Error: Invalid message format");
            }
            // xQueueReset(queue);
        }

        /* Read ADC value */
        uint16_t duty_value = adc1_get_raw(ADC_CHANNEL_2);
        if (duty_value == 0xFFFF)
        {
            ESP_LOGE(TAG, "Error reading ADC");
            continue;
        }
        /* Convert ADC value to voltage */
        float voltage = (duty_value * 3.3) / 4095;
        float temperature = voltage_to_temp(voltage);

        /* Display temperature on LEDs */
        if (temp_on)
        {
            ESP_LOGI("INFO", "temperature %.1f", temperature);
            vTaskDelay(pdMS_TO_TICKS(1300));
        }

        /* Update LEDs based on temperature ranges */
        update_led(LED2.CHANEL_R, "RED", (temperature >= temp_minr && temperature <= temp_maxr) ? 150 : 0);
        update_led(LED2.CHANEL_G, "GREEN", (temperature >= temp_minG && temperature <= temp_maxG) ? 150 : 0);
        update_led(LED2.CHANEL_B, "BLUE", (temperature >= temp_minB && temperature <= temp_maxB) ? 150 : 0);

        vTaskDelay(pdMS_TO_TICKS(600)); /* Delay to avoid busy-waiting */
    }
}

/* Function to update LED duty cycle
@brief
    Update LED duty cycle

@param channel
    LED channel
@param channel_name
    Name of the LED channel
@param duty
    Duty cycle value
*/

void update_led(ledc_channel_t channel, const char *channel_name, int duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
    // ESP_LOGI(TAG, "UPDATE %s TO: %d", channel_name, duty);
}

/* Function to update duty cycle for a specific LED
@brief
    Update duty cycle for a specific LED

@param led
    Pointer to the LED structure
@param flags
    Flags to indicate which LED to update
@param duty
    Duty cycle value
*/

void update_duty(LED *led, uint16_t flags, uint16_t duty)
{
    switch (flags)
    {
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

/* Task to read ADC value and update LED duty cycle
@brief
    Task to read ADC value and update LED duty cycle

@param args
    Pointer to the task arguments
*/

void task_adc(void *args)
{

    while (1)
    {
        uint16_t duty_value = adc_read_smooth();
        update_duty(&LED1, flags, duty_value);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Function to initialize LEDs
@brief
    Function to initialize LEDs
*/
void led_init(void)
{
    LED1 = (LED){
        .CHANEL_R = LEDC_CHANNEL_0, .CHANEL_G = LEDC_CHANNEL_1, .CHANEL_B = LEDC_CHANNEL_2, .PIN_R = 5, .PIN_G = 18, .PIN_B = 19, .Duty_R = 0, .Duty_G = 0, .Duty_B = 0};
    LED2 = (LED){
        .CHANEL_R = LEDC_CHANNEL_3, .CHANEL_G = LEDC_CHANNEL_4, .CHANEL_B = LEDC_CHANNEL_5, .PIN_R = 14, .PIN_G = 13, .PIN_B = 12, .Duty_R = 0, .Duty_G = 0, .Duty_B = 0};
    configurar_led(&LED1);
    configurar_led(&LED2);
    ESP_LOGI(TAG, "LEDs initialized successfully");
}

/* Function to process commands
@brief
    Function to process commands

@param command
    Command to process
*/

void process_command(char *command)
{
    if (command == NULL)
    {
        ESP_LOGE(TAG, "Error: Command is NULL");
        return;
    }
    /* Tokenize the command string */
    char *token1 = strtok(command, " ");
    if (token1 == NULL)
    {
        // ESP_LOGE(TAG, "Error: No command received");
        return;
    }
    ESP_LOGI("INFO", "token1 %s", token1);

    /* Handle temp_on and temp_off commands */
    if (strstr(token1, "temp_on") != NULL)
    {
        temp_on = 1;
        ESP_LOGI(TAG, "Temperature display turned ON");
        return;
    }
    if (strstr(token1, "temp_off") != NULL)
    {
        temp_on = 0;
        ESP_LOGI(TAG, "Temperature display turned OFF");
        return;
    }

    /*Verify if first word is 'm' or 'x'*/
    if (strcmp(token1, "m") != 0 && strcmp(token1, "x") != 0)
    {
        ESP_LOGE(TAG, "Error: First word must be 'm' or 'x'");
        return;
    }
    /*Verify if second word is 'R', 'G', or 'B'*/
    char *token2 = strtok(NULL, " ");
    if (token2 == NULL || (strcmp(token2, "R") != 0 && strcmp(token2, "G") != 0 && strcmp(token2, "B") != 0))
    {
        ESP_LOGE(TAG, "Error: Second word must be 'R', 'G', or 'B'");
        return;
    }
    /*Verify if third word is a number between 0 and 150*/
    char *token3 = strtok(NULL, " ");
    if (token3 == NULL)
    {
        ESP_LOGE(TAG, "Error: No value provided");
        return;
    }
    /*Convert string to integer*/
    int value = atoi(token3);
    if (value < 0 || value > 150)
    {
        ESP_LOGE(TAG, "Error: Value must be between 0 and 150");
        return;
    }

    ESP_LOGI(TAG, "Valid command: %s %s %d", token1, token2, value);

    /*Allocate memory for the message*/
    char *message = malloc(20);
    if (message == NULL)
    {
        ESP_LOGE(TAG, "Error: Failed to allocate memory");
        return;
    }

    /*Format the message*/
    snprintf(message, 20, "%s %s %d", token1, token2, value);

    /*Send the message to the queue*/
    if (xQueueSend(queue, message, pdMS_TO_TICKS(100)) == pdPASS)
    {
        ESP_LOGI(TAG, "Sender: Sent '%s'", message);
    }
    else
    {
        ESP_LOGE(TAG, "Sender: Failed to send '%s'", message);
    }

    /*Free the allocated memory*/
    free(message);
}

/*Function to process commands
@brief
    Function to process commands

@param command
    Command to process

*/

void verify_entry(void)
{
    char *data = readuart();
    if (data != NULL)
    {
        // ESP_LOGI(TAG, "Data received from UART: %s", data);
        process_command(data);
        free(data);
    }
    data = NULL;
}

/*Main application function*/

void app_main(void)
{

    isr_install();
    adc_config();
    adc_config2();
    led_init();
    init_uart0();
    /*Create queue*/
    queue = xQueueCreate(10, sizeof(char[20]));
    if (queue == NULL)
    {
        ESP_LOGE(TAG, "Error creating queue");
        return;
    }
    /*Create ADC task*/
    esp_err_t ret = xTaskCreatePinnedToCore(task_adc, "task_adc", 4096, NULL, 2, NULL, 0);
    if (ret != pdPASS)
    {
        ESP_LOGE(TAG, "Error creating ADC task: %d", ret);
        return;
    }
    /*Create range task*/
    esp_err_t ret2 = xTaskCreatePinnedToCore(task_range, "task_range", 4096, NULL, 3, NULL, 1);
    if (ret2 != pdPASS)
    {
        ESP_LOGE(TAG, "Error creating range task: %d", ret2);
        return;
    }
    /*Loop forever*/
    while (1)
    {
        verify_entry();
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay to avoid busy-waiting
    }
}