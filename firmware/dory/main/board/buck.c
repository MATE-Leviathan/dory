#include "buck.h"

#include "driver/gpio.h"
#include "esp_err.h"

#include "board_pins.h"

void buck_init(void)
{
    ESP_ERROR_CHECK(gpio_set_direction(EN_5V_PIN, GPIO_MODE_OUTPUT));
    buck_5v_set(false);
}

void buck_5v_set(bool on)
{
    gpio_set_level(EN_5V_PIN, on);
}
