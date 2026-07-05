#pragma once

#include <stdbool.h>

#include "dory_protocol.h"

void init_com(void);
bool send_result(const result_payload_t *result);
