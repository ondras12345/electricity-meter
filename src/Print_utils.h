#pragma once

#include <Print.h>

void Print_hexdump(Print *response, const uint8_t *data, size_t len, size_t offset = 0);
