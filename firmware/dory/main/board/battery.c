#include "battery.h"

#include <stdbool.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"

static const char *TAG = "battery";

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle;
static bool adc_calibrated;

void battery_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, BATT_ADC_CHANNEL, &chan_cfg));

    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .chan = BATT_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_calibrated = adc_cali_create_scheme_curve_fitting(&cali_cfg, &adc_cali_handle) == ESP_OK;
}

float battery_voltage_read(void)
{
    const int samples = 32;
    int mv_sum = 0;

    for (int i = 0; i < samples; i++) {
        int raw = 0;
        int mv = 0;

        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, BATT_ADC_CHANNEL, &raw));
        if (adc_calibrated) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw, &mv));
        } else {
            mv = raw * 3300 / 4095;
        }

        mv_sum += mv;
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    return (mv_sum / (float)samples / 1000.0f) * BATT_DIVIDER_MULTIPLIER;
}

uint16_t battery_mv_read(void)
{
    float volts = battery_voltage_read();
    uint16_t millivolts = (uint16_t)(volts * 1000.0f);

    ESP_LOGI(TAG, "Battery: %u mV", millivolts);
    return millivolts;
}
