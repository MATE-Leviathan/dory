#pragma once

#include "esp_err.h"

typedef struct {
    float temperature_c;
    float pressure_pa;
    float depth_m;
} bar02_sensor_sample_t;

esp_err_t bar02_sensor_init(void);

esp_err_t bar02_sensor_read(bar02_sensor_sample_t *sample);
