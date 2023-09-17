#pragma once
#include <stdint.h>

typedef struct {
    /// seconds since 2000-01-01
    uint32_t timestamp;
    /**
     * Number of pulses counted during this
     * one minute (increment).
     * 0xFFFF is not a valid pulse count (way too much,
     * see notes/SPI-flash.ipynb).
     */
    uint16_t pulses;
} datalogger_record_t;

void datalogger_init();
void datalogger_loop();
