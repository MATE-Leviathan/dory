#include "light.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#include "board_pins.h"

static const char *TAG = "light";
static bool s_light_on;

void light_init(void)
{
    ESP_ERROR_CHECK(gpio_set_direction(LIGHT_PIN, GPIO_MODE_OUTPUT));
    light_set(false);
    ESP_LOGI(TAG, "Light initialized");
}

void light_set(bool on)
{
    gpio_set_level(LIGHT_PIN, on);
    s_light_on = on;
    ESP_LOGI(TAG, "Light %s", on ? "on" : "off");
}

bool light_is_on(void)
{
    return s_light_on;
}
