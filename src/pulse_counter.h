#pragma once
#include <stdint.h>
#include <stdbool.h>

void pulse_counter_init();
void pulse_counter_loop();
uint32_t pulse_counter_get_count();
