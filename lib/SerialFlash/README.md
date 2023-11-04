# SerialFlash
- https://github.com/PaulStoffregen/SerialFlash/
- 8c310813003388455b8738bddda34eb03f3a3ef3
- I had to patch `SerialFlashChip.cpp` - I reduced SPI clock speed to
  18MHz, because STM32F103 with 72MHz clock and prescaler=2 results in
  36MHz SPI clock speed, but according to the datasheet, the SPI
  peripheral can only do 18MHz.
