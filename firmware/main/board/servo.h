#pragma once

#include <stdbool.h>

void servo_init(void);
void set_power(bool on);
bool servo_power_is_on(void);
