#include "servo.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#include "board_pins.h"

static const char *TAG = "servo";
static bool s_power_on;

void servo_init(void)
{
    ESP_ERROR_CHECK(gpio_set_direction(EN_5V_PIN, GPIO_MODE_OUTPUT));
    set_power(false);
    ESP_LOGI(TAG, "Servo power initialized");
}

void set_power(bool on)
{
    gpio_set_level(EN_5V_PIN, on);
    s_power_on = on;
    ESP_LOGI(TAG, "Servo 5V power %s", on ? "on" : "off");
}

bool servo_power_is_on(void)
{
    return s_power_on;
}
