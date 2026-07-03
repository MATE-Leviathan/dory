#include "light.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#include "board_pins.h"

static const char *TAG = "light";

void light_init(void)
{
    ESP_ERROR_CHECK(gpio_set_direction(LIGHT_PIN, GPIO_MODE_OUTPUT));
    light_set(false);
    ESP_LOGI(TAG, "Light initialized");
}

void light_set(bool on)
{
    gpio_set_level(LIGHT_PIN, on);
    ESP_LOGI(TAG, "Light %s", on ? "on" : "off");
}
