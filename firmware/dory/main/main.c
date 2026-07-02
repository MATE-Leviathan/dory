#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "board/battery.h"
#include "board/servo.h"
#include "board/buzzer.h"
#include "board/light.h"
#include "debug_serial.h"
#include "nvs_flash.h"

void init_com(void);

void app_main(void)
{
    servo_init();
    light_init();
    buzzer_init();
    battery_init();

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
