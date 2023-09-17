#include "hardware.h"

void hardware_init()
{
    pinMode(PIN_LED_STATUS, OUTPUT);
    LED_STATUS(false);
    pinMode(PIN_LED_PULSE, OUTPUT);
    LED_PULSE(false);
    pinMode(PIN_LED_FILTERED, OUTPUT);
    LED_FILTERED(false);

    pinMode(PIN_BTN_UMOUNT, INPUT);
    pinMode(PIN_DIP_SW_1, INPUT_PULLDOWN);
    pinMode(PIN_DIP_SW_2, INPUT_PULLDOWN);

    // TODO MODBUS
    pinMode(PIN_MODBUS_DE, OUTPUT);
    // receive mode
    digitalWrite(PIN_MODBUS_DE, LOW);

    // TODO SPI flash
    pinMode(PIN_SPI_SS_FLASH, OUTPUT);
    digitalWrite(PIN_SPI_SS_FLASH, HIGH);

    pinMode(PIN_PULSE_COUNTER, INPUT);
    analogReadResolution(ADC_RESOLUTION_BITS);
}
