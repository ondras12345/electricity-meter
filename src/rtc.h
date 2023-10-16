#pragma once
#include <stdint.h>
#include <RTClib.h>

void rtc_init();
void rtc_loop();


void rtc_refresh();
bool rtc_set_datetime(uint8_t year, uint8_t month, uint8_t day, uint8_t hours, uint8_t minutes, uint8_t seconds);

extern DateTime rtc_time;
