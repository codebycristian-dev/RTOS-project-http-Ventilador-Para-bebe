idf_component_register(SRCS "main.c" "wifi_app.c" "http_server.c" "sensor_app.c"  "fan_control.c" "config_app.c" "logic_app.c" "ds18b20_bitbang.c" "onewire.c"
                    PRIV_REQUIRES driver esp_wifi nvs_flash driver esp_http_server esp_timer app_update json mqtt freertos

                    INCLUDE_DIRS "."
                    EMBED_FILES webpage/app.css webpage/app.js webpage/favicon.ico webpage/index.html webpage/jquery-3.3.1.min.js 
                    )
 