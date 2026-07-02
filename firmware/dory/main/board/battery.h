#pragma once

#include <stdint.h>

void battery_init(void);
float battery_voltage_read(void);
uint16_t battery_mv_read(void);
