#include "cli.h"
#include <Shellminator.hpp>
#include <Shellminator-IO.hpp>
#include <Commander-API.hpp>
#include <Commander-API-Commands.hpp>
#include <Commander-IO.hpp>
#include "debug.h"
#include "version.h"
#include "pulse_counter_advanced.h"
#include "rtc.h"


static Shellminator shell(&Serial);
static Commander commander;


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
    response->print("  min: ");
    response->println(ap.min);
    response->print("  max: ");
    response->println(ap.max);
}


static void cmnd_rtc(char *args, Stream *response)
{
    const char * subcommand = strsep(&args, " ");
    if (subcommand == nullptr)
    {
        // this is ok, just print time and exit
    }
    else if (strcmp(subcommand, "set"))
    {
        unsigned int year, month, day, hours, minutes, seconds;
        if (sscanf(args, "%04u-%02u-%02uT%02u:%02u:%02uZ",
                   &year, &month, &day, &hours, &minutes, &seconds))
        {
            response->println("error: could not parse args");
            goto bad;
        }
        if (!rtc_set_date(day, month, year))
        {
            response->println("error: invalid (illogical) date");
            goto bad;
        }
        rtc_set_time(hours, minutes, seconds);
    }

    rtc_refresh();

    response->printf(
        "time: 20%02u-%02u-%02uT%02u:%02u:%02uZ\r\n",
        rtc_year, rtc_month, rtc_day,
        rtc_hours, rtc_minutes, rtc_seconds
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


static Commander::API_t API_tree[] = {
    apiElement("reset",         "Reset the MCU.",                           cmnd_reset),
    apiElement("ver",           "Print out version info.",                  cmnd_ver),
    apiElement("pulse",         "Get pulse counter status.",                cmnd_pulse),
    apiElement("rtc",           "Get or set RTC time.",                     cmnd_rtc),
    apiElement("speedtest",     "Test Serial speed.",                       cmnd_speedtest),
    // commander pre-made commands
    API_ELEMENT_MILLIS,
    API_ELEMENT_UPTIME,
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
}


void cli_loop()
{
    shell.update();
}
