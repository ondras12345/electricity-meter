#include "datalogger.h"
#include <stdint.h>
#include <Arduino.h>
#include "pulse_counter.h"
#include "rtc.h"
#include "debug.h"


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


static void write_record(datalogger_record_t record)
{
    // prevent invalid pulse count
    if (record.pulses == (uint16_t)-1) record.pulses = 0;

    // TODO write to flash
    DEBUG->print("datalogger record: timestamp=");
    DEBUG->print(record.timestamp);
    DEBUG->print(" pulses=");
    DEBUG->println(record.pulses);
}


void datalogger_init()
{
    // TODO SPI flash
}


void datalogger_loop()
{
    static unsigned long prev_millis = 0;
    unsigned long now = millis();
    bool too_early = now - prev_millis <= 1500UL;
    bool too_late = now - prev_millis >= 62000UL;
    if ((rtc_seconds == 0 && !too_early) || too_late)
    {
        prev_millis = now;
        datalogger_record_t record;
        record.timestamp = rtc_epoch_2000;
        uint32_t pulses = pulse_counter_get_count();
        static uint32_t prev_pulses = 0;
        record.pulses = pulses-prev_pulses;
        prev_pulses=pulses;
        write_record(record);
    }
}
