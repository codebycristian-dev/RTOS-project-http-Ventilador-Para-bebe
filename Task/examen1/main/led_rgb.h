#ifndef LEDCUSER_H
#define LEDCUSER_H

#include "driver/ledc.h"
#include "esp_err.h"

/*Define pins for LED RGB*/
#define PINBR 5
#define PINBG 18
#define PINBB 19
#define PINBUZZER 21
#define CHBUZZER LEDC_CHANNEL_3

/*Define structure for LED RGB*/
typedef struct
{
    ledc_channel_t CHANEL_R; /*Channel for red color*/
    ledc_channel_t CHANEL_G; /*Channel for green color*/
    ledc_channel_t CHANEL_B; /*Channel for blue color*/

    uint16_t PIN_R; /*Pin GPIO for red color*/
    uint16_t PIN_G; /*Pin GPIO for green color*/
    uint16_t PIN_B; /*Pin GPIO for blue color*/

    uint16_t Duty_R; /*Value of duty cycle for red color*/
    uint16_t Duty_G; /*Value of duty cycle for green color*/
    uint16_t Duty_B; /*Value of duty cycle for blue color*/
} LED;

/*Define structure for normal LED*/
typedef struct
{
    ledc_channel_t CHANNEL; /*Channel for PWM*/
    uint16_t PIN;           /*Pin GPIO for output*/
    uint16_t DUTY;          /*Duty cycle for PWM output*/
} NORMAL_LED;

extern LED LED1;
extern NORMAL_LED Buzzer;

/*Declaration of functions*/
void configurar_led(LED *led);
void configurar_normal_led(NORMAL_LED *normal_led);
void initfastLED(void);

#endif