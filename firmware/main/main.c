#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "board/battery.h"
#include "board/bar02_sensor.h"
#include "board/servo.h"
#include "board/buzzer.h"
#include "board/light.h"
#include "com.h"
#include "debug_serial.h"

void app_main(void)
{
    servo_init();
    light_init();
    buzzer_init();
    battery_init();

    light_set(true);
    buzzer_play_samsung_boot();
    light_set(false);

    esp_err_t sensor_err = bar02_sensor_init();
    if (sensor_err != ESP_OK) {
        ESP_LOGW("main", "Bar02 sensor unavailable: %s", esp_err_to_name(sensor_err));
        buzzer_play_sensor_missing();
    }

#ifdef CONFIG_DORY_DEBUG_SERIAL
    debug_serial_start();
#endif

#ifdef CONFIG_DORY_WIRELESS
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    init_com();
#endif

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
