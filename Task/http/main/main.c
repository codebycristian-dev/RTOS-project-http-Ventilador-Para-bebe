/**
 * Application entry point.
 */

#include "nvs_flash.h"
// #include "http_server.h"
#include "wifi_app.h"
#include "driver/gpio.h"
#include "fan_control.h"
#include "sensor_app.h"
#include "config_app.h"
#include "logic_app.h"

#define BLINK_GPIO 2
/**
 * @brief Configura el pin del LED para parpadeo.
 */
static void configure_led(void)
{

	gpio_reset_pin(BLINK_GPIO);
	gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}
/**
 * @brief Punto de entrada principal de la aplicación.
 */
void app_main(void)
{
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Start Wifi
	init_obtain_time();
	sensor_app_init();
	fan_control_init();
	fan_set_pwm(0); // Apagado por defecto
	config_app_init();
	logic_app_start();
	configure_led();
	wifi_app_start();
	
}
