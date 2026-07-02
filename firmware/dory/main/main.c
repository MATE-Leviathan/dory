#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board/battery.h"
#include "board/buck.h"
#include "board/buzzer.h"
#include "board/light.h"

void app_main(void)
{
    buck_init();
    light_init();
    buzzer_init();
    battery_init();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
