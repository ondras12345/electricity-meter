#include "datalogger.h"
#include <Arduino.h>
#include <SerialFlash.h>
#include "hardware.h"
#include "pulse_counter.h"
#include "rtc.h"
#include "debug.h"


// erasable files must be aligned to 64k (I think)
#define DATA_FILE_LENGTH (128*1024U)
// 64 * 64kB max for 32Mbit flash
// 32 * 256kB
// filesystem needs some space, too
// leaving extra space for possible future config file
#define DATA_FILE_COUNT 31
#define DATA_FILE_RECORDS (DATA_FILE_LENGTH / sizeof(datalogger_record_t))


static uint_fast8_t datalogger_current_file_index = -1;  // -1 means invalid
static uint32_t datalogger_current_index = 0;


static void ensure_erasable_file(const char * filename, uint32_t length)
{
    if (!SerialFlash.exists(filename))
    {
        bool retry = false;
tryagain:
        if (!SerialFlash.createErasable(filename, length))
        {
            if (retry)
            {
                Serial.println("Failed to create file, even after chip erase");
                return;
            }
            DEBUG->print("Failed to create file: ");
            DEBUG->println(filename);
            SerialFlash.eraseAll();
            while (!SerialFlash.ready())
            {
                // TODO reset WDT ?
            }
            retry = true;
            goto tryagain;
        }
    }
}


static const char * number_to_filename(uint8_t i)
{
    static char filename[3];
    if (i >= DATA_FILE_COUNT) return "invalid";
    snprintf(filename, sizeof filename, "%02u", i);
    return filename;
}


static void write_record(datalogger_record_t record)
{
    if (datalogger_current_file_index == (uint_fast8_t)-1)
    {
        // no valid file
        DEBUG->println("error: no file to log to");
        return;
    }

    // prevent invalid pulse count
    if (record.pulses == (uint16_t)-1) record.pulses = 0;

    DEBUG->print("datalogger record: timestamp=");
    DEBUG->print(record.timestamp);
    DEBUG->print(" pulses=");
    DEBUG->println(record.pulses);

    // We need to erase next file BEFORE writing last record.
    // If power failed while writing, we need to be able to find one file
    // where the last record is invalid.
    uint_fast8_t next_file = (datalogger_current_file_index + 1) % DATA_FILE_COUNT;
    bool is_last_record = datalogger_current_index == DATA_FILE_RECORDS - 1;
    if (is_last_record)
    {
        const char * filename = number_to_filename(next_file);
        DEBUG->print("erasing next file: ");
        DEBUG->println(filename);
        SerialFlashFile f = SerialFlash.open(filename);
        if (!f)
        {
            DEBUG->println("error: could not open");
            // let's try again next time...
            return;
        }
        f.erase();
    }
    SerialFlashFile f = SerialFlash.open(number_to_filename(datalogger_current_file_index));
    f.seek(datalogger_current_index * sizeof(datalogger_record_t));
    f.write(&record, sizeof record);
    if (is_last_record)
    {
        datalogger_current_index = 0;
        datalogger_current_file_index = next_file;
    }
}


void datalogger_init()
{
    DEBUG->print("Flash init: ");
    DEBUG->println(SerialFlash.begin(PIN_SPI_SS_FLASH));

    DEBUG->println("creating files...");
    for (uint_fast8_t i = 0; i < DATA_FILE_COUNT; i++)
    {
        ensure_erasable_file(number_to_filename(i), DATA_FILE_LENGTH);
    }

    DEBUG->println("searching for last record");
    // Read the last record of each file, search for pulses=0xFFFF.
    // We are only doing this once at startup, and there aren't that many
    // files. It should be OK to just open & read each one.
    uint_fast8_t current_file = -1;
    SerialFlashFile f;
    for (uint_fast8_t i = 0; i < DATA_FILE_COUNT; i++)
    {
        const char * filename = number_to_filename(i);
        DEBUG->print("file: ");
        DEBUG->println(filename);
        f = SerialFlash.open(filename);
        if (!f)
        {
            // This should never happen, we called ensure_erasable_file before.
            DEBUG->println("error: file does not exist");
            continue;
        }
        datalogger_record_t last_record;
        f.seek((DATA_FILE_RECORDS - 1) * sizeof(last_record));
        f.read(&last_record, sizeof last_record);
        if (last_record.pulses == 0xFFFF)
        {
            DEBUG->println("empty at the end");
            current_file = i;
            break;
        }
        // SerialFlash files are not real files, no need to close()
    }

    if (!f)
    {
        DEBUG->println("error: current_file does not exist");
        return;
    }

    datalogger_current_file_index = current_file;

    // Binary search within current_file
    uint32_t left = 0;
    uint32_t right = DATA_FILE_RECORDS - 1;
    uint32_t last_valid = -1;
    while (left <= right)
    {
        uint32_t mid = (left + right) / 2;
        datalogger_record_t record;
        f.seek(mid * sizeof(record));
        f.read(&record, sizeof(record));
        if (record.pulses != 0xFFFF)
        {
            last_valid = mid;
            left = mid+1;
        }
        else right = mid-1;
    }
    // If no valid record was found, last_valid was initialized to
    // (uint32_t)(-1), so last_valid+1 == 0 and everything still works.
    datalogger_current_index = last_valid+1;
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
