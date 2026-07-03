#pragma once

#include "driver/gpio.h"
#include "hal/adc_types.h"

#define EN_5V_PIN GPIO_NUM_16
#define BATT_ADC_PIN GPIO_NUM_4
#define BATT_ADC_CHANNEL ADC_CHANNEL_3
#define LIGHT_PIN GPIO_NUM_9
#define BUZZER_PIN GPIO_NUM_48

// 220k and 100k voltage divider
#define BATT_DIVIDER_MULTIPLIER 3.2f
