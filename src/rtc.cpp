#include "rtc.h"
#include <STM32RTC.h>
#include <Arduino.h>
#include <RTClib.h>
#include "debug.h"

static STM32RTC& rtc = STM32RTC::getInstance();

uint8_t rtc_hours = 0;
uint8_t rtc_minutes = 0;
uint8_t rtc_seconds = 0;
uint32_t rtc_subSeconds = 0;
uint8_t rtc_day = 0;
uint8_t rtc_month = 0;
uint8_t rtc_year = 0;
uint32_t rtc_epoch_2000 = 0;


void rtc_init()
{
    DEBUG->println("Init RTC...");
    DEBUG->printf("clock source was: %d\r\n", rtc.getClockSource());
    // Use 32.768kHz crystal oscillator.
    // During testing, I found that parasitic capacitance (such as
    // from a breadboard) can cause the oscillator to be significantly
    // off frequency.
    rtc.setClockSource(STM32RTC::LSE_CLOCK);
    rtc.setPrediv(32768U);
    rtc.begin();
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
    rtc.getTime(&rtc_hours, &rtc_minutes, &rtc_seconds, &rtc_subSeconds);
    // we don't care about weekday
    rtc.getDate(nullptr, &rtc_day, &rtc_month, &rtc_year);
    rtc_epoch_2000 = rtc.getY2kEpoch();
}


void rtc_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    // rtc.setTime does error checking internally
    rtc.setTime(hours, minutes, seconds);
}


/**
 * Set RTC date.
 * @param year 0-99 (20xx)
 * @return true on success, false if date is invalid
 */
bool rtc_set_date(uint8_t day, uint8_t month, uint8_t year)
{
    DateTime dt(year, month, day);
    if (!dt.isValid()) return false;
    // we don't need weekday
    rtc.setDate(day, month, year);
    return true;
}
