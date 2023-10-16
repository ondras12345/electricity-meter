/*
 * STM32F103 bluepill PCBs seem to have 20ppm crystals for LSE clock.
 * That's way to imprecise for RTC. Also, the chips errata lists some problems:
 * - LSE clock sometimes does not start
 * - There was a software bug that causes the clock to loose 0.5s after each
 *   reset (I'm not sure if it is fixed in STM32FTC.h / stm32duino).
 * - Use of PC13 can cause LSE clock to loose cycles.
 *
 * I think external DS3231 RTC is a much better choice.
 */

#include "rtc.h"
#include <Arduino.h>
#include "debug.h"

DateTime rtc_time;

static RTC_DS3231 rtc;


void rtc_init()
{
    DEBUG->print("RTC init:");
    DEBUG->println(rtc.begin());

    if (rtc.lostPower())
    {
        DEBUG->println("RTC lost power");
        rtc.adjust(DateTime(2000,1,1,0,0,0));
    }

    rtc_refresh();
}


void rtc_loop()
{
    static unsigned long prev_millis = -1000;  // refresh immediately after boot
    unsigned long now = millis();
    if (now - prev_millis >= 500UL)
    {
        prev_millis = now;
        rtc_refresh();
    }
}


void rtc_refresh()
{
    rtc_time = rtc.now();
}


/**
 * Set RTC date.
 * @param year 0-99 (20xx)
 * @return true on success, false if date is invalid
 */
bool rtc_set_datetime(uint8_t year, uint8_t month, uint8_t day, uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    DateTime dt(year, month, day, hours, minutes, seconds);
    if (!dt.isValid()) return false;
    rtc.adjust(dt);
    return true;
}


// TODO measure difference between RTC (LSE clock) & millis?
