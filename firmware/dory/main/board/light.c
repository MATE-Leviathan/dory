#include "light.h"

#include "driver/gpio.h"
#include "esp_err.h"

#include "board_pins.h"

void light_init(void)
{
    ESP_ERROR_CHECK(gpio_set_direction(LIGHT_PIN, GPIO_MODE_OUTPUT));
    light_set(false);
}

void light_set(bool on)
{
    gpio_set_level(LIGHT_PIN, on);
}
