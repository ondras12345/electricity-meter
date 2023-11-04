#pragma once
#include <Arduino.h>

#define PIN_DIP_SW_1 PB0
#define PIN_DIP_SW_2 PB1

#define PIN_LED_STATUS PB12
#define PIN_LED_PULSE PB13
#define PIN_LED_FILTERED PC13

#define PIN_BTN_1 PB14

// see also platformio.ini
#define PIN_RS485_DE PA1

#define PIN_SPI_SS_FLASH PA4
#define PIN_PULSE_COUNTER PA2

#define ADC_RESOLUTION_BITS 12


// LEDs are active low
#define LED_STATUS(state) digitalWrite(PIN_LED_STATUS, !state)
#define LED_PULSE(state) digitalWrite(PIN_LED_PULSE, !state)
#define LED_FILTERED(state) digitalWrite(PIN_LED_FILTERED, !state)

#define BUTTON_1 (!digitalRead(PIN_BTN_1))
#define SWITCH_DIP_1 digitalRead(PIN_DIP_SW_1)
#define SWITCH_DIP_2 digitalRead(PIN_DIP_SW_2)

void hardware_init();
