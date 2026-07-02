#include "servo.h"

#include "driver/gpio.h"
#include "esp_err.h"

#include "board_pins.h"

void servo_init(void)
{
    ESP_ERROR_CHECK(gpio_set_direction(EN_5V_PIN, GPIO_MODE_OUTPUT));
    set_power(false);
}

void set_power(bool on)
{
    gpio_set_level(EN_5V_PIN, on);
}
