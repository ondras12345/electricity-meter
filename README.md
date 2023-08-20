# Electricity meter
- RTC
  - internal? (low stability)
    - DCF77 receiver?
  - external DS3231?
- UART / USB
  - USB-CDC is way faster
- 1x ADC for pulse input
  - automatic threshold - see [`notes/automatic-threshold.ipynb`](notes/automatic-threshold.ipynb)
- umount button
- LED showing counted pulses
- state indicator LED
- SPI flash || SD card || both
  - TODO
- DIP switches (pull-up value)
  - TODO make it software configurable instead? (only VCC-0.4V)
- ATmega8 ?
  - - needs external RTC
  - - too little flash space
  - - needs 3v3 converter for SPI flash & SD card
  - + lots of them
  - usart can go faster https://arduino.stackexchange.com/questions/296/how-high-of-a-baud-rate-can-i-go-without-errors
- STM32F103
  - - more expensive
  - - power consumption?
  - - physically bigger
  - + has RTC (precision?, external crystal?)
  - + USB CDC
  - + higher ADC resolution
  - + 3v3 SD card
  - + enough flash for fully featured command line
- RS485 modbus? - would need optical isolation
