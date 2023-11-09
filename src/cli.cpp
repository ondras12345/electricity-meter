#include "cli.h"
#include <Shellminator.hpp>
#include <Shellminator-IO.hpp>
#include <Commander-API.hpp>
#include <Commander-API-Commands.hpp>
#include <Commander-IO.hpp>
#include <SerialFlash.h>
#include <ArduinoRS485.h>
#include "Print_utils.h"
#include "debug.h"
#include "version.h"
#include "pulse_counter_advanced.h"
#include "rtc.h"
#include "hardware.h"
#include "datalogger.h"


static Shellminator shell(&Serial);
static Commander commander;
static bool RS485_enable;


static void cmnd_reset(char *args, Stream *response)
{
    response->println("Resetting");
    response->flush();
    delay(100);
    NVIC_SystemReset();
}


static void cmnd_ver(char *args, Stream *response)
{
    response->print("version: ");
    response->println(ELECTRICITY_METER_VERSION);
    response->print("build time: ");
    response->println(ELECTRICITY_METER_BUILD_TIME);
}


static void cmnd_pulse(char *args, Stream *response)
{
    analogpulse_t ap = pulse_counter_get_analogpulse();
    response->println("configuration:");
    response->print("  min_amplitude: ");
    response->println(ap.min_amplitude);
    response->print("  decay_speed: ");
    response->println(ap.decay_speed);
    response->println("state:");
    response->print("  count: ");
    response->println(pulse_counter_get_count());
    response->print("  valid: ");
    response->println(analogpulse_valid(&ap));
    response->print("  sample: ");
    response->println(ap.sample);
    response->print("  threshold: ");
    response->println(analogpulse_threshold(&ap));
    response->print("  digital: ");
    response->println(ap.digital);
    response->printf(
        "  min: %u.%02u (%" PRIu32 ")\r\n",
        ap.min/256,
        (ap.min % 256)*100 / 256,
        ap.min
    );
    response->printf(
        "  max: %u.%02u (%" PRIu32 ")\r\n",
        ap.max/256,
        (ap.max % 256)*100 / 256,
        ap.max
    );
    response->print("  amplitude: ");
    if (ap.max > ap.min)
    {
        uint32_t amplitude = ap.max - ap.min;
        const uint16_t max_reading = (1<<ADC_RESOLUTION_BITS) - 1;
        uint32_t percentage = amplitude / 256 * 10000 / max_reading;
        response->printf(
            "%u.%02u%%\r\n",
            percentage / 100,
            percentage % 100
        );
    }
    else response->println('-');
}


static void cmnd_rtc(char *args, Stream *response)
{
    const char * subcommand = strsep(&args, " ");
    if (subcommand == nullptr)
    {
        // this is ok, just print time and exit
    }
    else if (strcmp(subcommand, "set") == 0)
    {
        unsigned int year, month, day, hours, minutes, seconds;
        char c;  // we should NOT find any extra chars
        char Z;  // 'Z' in ISO8601 datetime
        if (sscanf(args, "%04u-%02u-%02uT%02u:%02u:%02u%c%c",
                   &year, &month, &day, &hours, &minutes, &seconds, &Z, &c) != 7)
        {
            response->println("error: could not parse args");
            goto bad;
        }
        if (Z != 'Z')
        {
            response->println("error: not a UTC datetime");
            goto bad;
        }
        if (year < 2000 || year > 3000)
        {
            response->println("error: invalid year");
            goto bad;
        }
        year -= 2000;
        if (!rtc_set_datetime(year, month, day, hours, minutes, seconds))
        {
            response->println("error: invalid (illogical) date");
            goto bad;
        }
    }
    else
    {
        response->println("error: invalid subcommand");
        goto bad;
    }

    rtc_refresh();

    response->printf(
        "time: %04u-%02u-%02uT%02u:%02u:%02uZ\r\n",
        rtc_time.year(), rtc_time.month(), rtc_time.day(),
        rtc_time.hour(), rtc_time.minute(), rtc_time.second()
    );
    return;
bad:
    response->println("usage: rtc [(set 2000-01-01T12:34:56Z)]");
}


static void cmnd_speedtest(char *args, Stream *response)
{
#define SPEEDTEST_SIZE 64*1024U
    unsigned long start = millis();
    for (unsigned int i = 0; i < SPEEDTEST_SIZE/16; i++)
    {
        response->print("................");
    }
    response->flush();
    unsigned long end = millis();
    response->println();
    response->print("64k ... ");
    response->print(end-start);
    response->print(" ms (");
    response->print(SPEEDTEST_SIZE*1UL*(8+2)*1000UL / (end-start));
    response->println(" baud)");
}


static void cmnd_SPIflash(char *args, Stream *response)
{
    const char * subcommand_name = strsep(&args, " ");
    char * subcommand_args = args;
    if (subcommand_name == nullptr)
    {
        // do nothing, just print out status
        response->print("ready: ");
        response->println(SerialFlash.ready());
        response->print("ID:");
        uint8_t id[5] = {0};
        SerialFlash.readID(id);
        for (uint_fast8_t i = 0; i < sizeof id; i++)
        {
            response->printf(" %02x", id[i]);
        }
        response->println();
        // Not supported by this chip (reads all FFs):
        //response->print("serial NR:");
        //uint8_t serial[8];
        //SerialFlash.readSerialNumber(serial);
        //for (uint8_t i = 0; i < sizeof serial; i++)
        //{
        //    response->printf(" %02x", serial[i]);
        //}
        //response->println();
        response->print("capacity: ");
        response->println(SerialFlash.capacity(id));
        response->print("block size: ");
        response->println(SerialFlash.blockSize());
    }

    // subcommands that need no args
    else if (strcmp(subcommand_name, "erase_chip") == 0)
    {
        SerialFlash.eraseAll();
        response->println("Erasing");
    }
    else if (strcmp(subcommand_name, "ls") == 0)
    {
        response->println("Files:");
        response->println("address (HEX)\tname\tsize (Bytes)");
        SerialFlash.opendir();
        char filename[32];
        uint32_t filesize;
        uint32_t address = 0;
        while (SerialFlash.readdir(filename, sizeof filename, filesize))
        {
            SerialFlashFile f = SerialFlash.open(filename);
            if (f)
            {
                address = f.getFlashAddress();
                f.close();
            }
            response->printf("%06x\t%s\t%u\r\n", address, filename, filesize);
        }
    }

    else if (subcommand_args == nullptr)
    {
        response->println("Missing subcommand args");
        goto bad;
    }

    // subcommands that need subcommand_args
    //else if (strcmp(subcommand_name, "rm") == 0)
    //{
    //    response->println("rm does NOT reclaim space, just filename");
    //    response->print("success: ");
    //    response->println(SerialFlash.remove(subcommand_args));
    //}
    else if (strcmp(subcommand_name, "create") == 0)
    {
        const char * filename = strsep(&subcommand_args, " ");
        const char * lengthstr = strsep(&subcommand_args, " ");
        const char * erasablestr = subcommand_args;
        if (filename == nullptr || lengthstr == nullptr || erasablestr == nullptr)
        {
            response->println("Invalid args");
            goto bad;
        }
        unsigned int length = 0;
        sscanf(lengthstr, "%u", &length);
        bool erasable = erasablestr[0] == '1';
        response->printf("Creating %sfile with name \"%s\" length %u\r\n",
                         erasable ? "erasable " : "", filename, length);
        response->print("success: ");
        response->println(
            erasable ? SerialFlash.createErasable(filename, length)
                     : SerialFlash.create(filename, length)
        );
    }
    else if (strcmp(subcommand_name, "dump") == 0)
    {
        const char * filename = strsep(&subcommand_args, " ");
        const char * startstr = subcommand_args;
        if (filename == nullptr || startstr == nullptr)
        {
            response->println("Invalid args");
            goto bad;
        }

        unsigned int start = strtoul(startstr, nullptr, 0);  // should accept hex 0x
        SerialFlashFile f = SerialFlash.open(filename);
        if (!f)
        {
            response->println("Cannot open file");
            return;
        }
        uint8_t buf[256];
        f.seek(start);
        Print_hexdump(response, buf, f.read(buf, sizeof buf), start);
        f.close();
    }
    else if (strcmp(subcommand_name, "erase") == 0)
    {
        SerialFlashFile f = SerialFlash.open(subcommand_args);
        if (!f)
        {
            response->println("Cannot open file");
            return;
        }
        response->println("(only files created as erasable can be erased)");
        f.erase();
        f.close();
    }
    else if (strcmp(subcommand_name, "dumpchip") == 0)
    {
        unsigned int start = strtoul(subcommand_args, nullptr, 0);  // should accept hex 0x
        uint8_t id[5];
        SerialFlash.readID(id);
        uint32_t capacity = SerialFlash.capacity(id);
        uint8_t buf[256];
        if (start >= capacity)
        {
            response->println("Address out of range");
            return;
        }
        uint32_t len = sizeof buf;
        if (len > capacity - start) len = capacity - start;
        SerialFlash.read(start, buf, len);
        Print_hexdump(response, buf, len, start);
    }
    else
    {
        response->println("Invalid subcommand");
        goto bad;
    }

    return;
bad:
    response->println(
"Usage: SPIflash [erase_chip | ls | create name length erasable | rm name |\r\n"
"                 dump name start | dumpchip start | erase filename]"
    );
}


static void cmnd_datalogger(char *args, Stream *response)
{
    // We should probably only read one file at a time to prevent blocking the
    // system for too long.
    // We might as well have the user enter the data file number.
    unsigned int fileno = 0;
    bool valid = (
        sscanf(args, "%u", &fileno) == 1
        && fileno < DATALOGGER_FILE_COUNT
    );
    SerialFlashFile f;  // needs to be declared before goto
    if (!valid)
    {
        response->println("invalid file_no");
        goto usage;
    }
    f = SerialFlash.open(datalogger_file_number_to_filename(fileno));
    if (!f)
    {
        // This should never happen
        response->println("e: could not open file");
        return;
    }
    response->println("---");
    response->println("# datetime\tenergy [1/10 Wh]");
    // It might be faster to read the whole page,
    // but I think Serial will be the bottleneck.
    // TODO verify
    for (uint32_t i = 0; i < DATALOGGER_FILE_RECORDS; i++)
    {
        datalogger_record_t record;
        f.read(&record, sizeof record);
        if (datalogger_record_empty(record))
        {
            response->println("# END OF VALID RECORDS");
            break;
        }
        DateTime t(record.timestamp + SECONDS_FROM_1970_TO_2000);
        response->printf(
            "%04u-%02u-%02uT%02u:%02u:%02uZ\t%u\r\n",
            t.year(), t.month(), t.day(),
            t.hour(), t.minute(), t.second(),
            record.pulses
        );
        // TODO this seems to fix USB CDC losing data.
        if (i % 50 == 0) delay(10);
    }
    response->println("---");
    return;
usage:
    response->println(
        "Usage: datalogger file_no"
    );
}


static Commander::API_t API_tree[] = {
    //apiElement("reset",         "Reset the MCU.",                           cmnd_reset),
    apiElement("ver",           "Print out version info.",                  cmnd_ver),
    apiElement("pulse",         "Get pulse counter status.",                cmnd_pulse),
    apiElement("rtc",           "Get or set RTC time.",                     cmnd_rtc),
    apiElement("speedtest",     "Test Serial speed.",                       cmnd_speedtest),
    apiElement("SPIflash",      "Issue commands to SPI flash.",             cmnd_SPIflash),
    apiElement("datalogger",    "Read datalogger records from SPI flash.",  cmnd_datalogger),
    // commander pre-made commands
    //API_ELEMENT_MILLIS,
    //API_ELEMENT_UPTIME,
    // digitalRead, analogRead look too primitive to be useful. They also don't
    // validate args, that could potentially be dangerous.
};


void cli_init()
{
    // Clear the terminal
    //shell.clear();

    //commander.attachDebugChannel(DEBUG);

    commander.attachTree(API_tree);
    commander.init();
    shell.attachCommander(&commander);
    shell.begin("elektromer");

    RS485_enable = digitalRead(PIN_DIP_SW_2);
    if (RS485_enable)
    {
        RS485.begin(9600);
        // TODO RS485 receive commands
        // (That would make debugging harder)
        //RS485.receive();
        RS485.beginTransmission();
        // TODO end transmission to reduce power consumption ??
    }
}


void cli_loop()
{
    shell.update();

    if (!RS485_enable) return;

    static unsigned long rs485_prev_report = 0;
    unsigned long now = millis();
    if (now - rs485_prev_report >= 1000UL)
    {
        rs485_prev_report = now;
        RS485.println("rtc:");
        commander.execute("rtc", &RS485);
        RS485.println("pulse:");
        commander.execute("pulse", &RS485);
    }
}
