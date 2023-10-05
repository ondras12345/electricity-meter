#pragma once
#include <stdint.h>
#include <RTClib.h>

void rtc_init();
void rtc_loop();


void rtc_refresh();
void rtc_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds);
bool rtc_set_date(uint8_t day, uint8_t month, uint8_t year);

extern DateTime rtc_time;
