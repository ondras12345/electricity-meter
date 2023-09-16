#include <Arduino.h>
#include "hardware.h"
#include "rtc.h"
#include "debug.h"
#include "pulse_counter.h"
#include "cli.h"
#include "datalogger.h"

#include <SPI.h>  // TODO testing RTClib


void setup()
{
    hardware_init();

    Serial.begin();

    // wait for serial to initialize
    for (uint_fast8_t i = 0; i < 4; i++)
    {
        LED_STATUS(true);
        delay(100);
        LED_STATUS(false);
        delay(100);
    }

    rtc_init();
    pulse_counter_init();
    datalogger_init();
    cli_init();

    LED_STATUS(true);
    DEBUG->println("boot");
    delay(500);
    LED_STATUS(false);
}


void loop()
{
    rtc_loop();
    pulse_counter_loop();
    datalogger_loop();
    cli_loop();
}
