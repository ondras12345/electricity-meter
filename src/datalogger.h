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
} __attribute__((packed)) datalogger_record_t;

const char * datalogger_file_number_to_filename(uint_fast8_t);

bool datalogger_record_empty(datalogger_record_t);

void datalogger_init();
void datalogger_loop();


// erasable files must be aligned to 64k (I think)
#define DATALOGGER_FILE_LENGTH (128*1024U)
// 64 * 64kB max for 32Mbit flash
// 32 * 256kB
// filesystem needs some space, too
// leaving extra space for possible future config file
#define DATALOGGER_FILE_COUNT 31
/// number of datalogger_record_t records in one file
#define DATALOGGER_FILE_RECORDS (DATALOGGER_FILE_LENGTH / sizeof(datalogger_record_t))
