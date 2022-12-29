#include "cli.h"
#include <Arduino.h>
#include <Shellminator.hpp>
#include <Shellminator-IO.hpp>
#include <Commander-API.hpp>
#include <Commander-IO.hpp>
#include <RTClib.h>

Shellminator shell(&Serial);
Commander commander;

extern RTC_DS3231 rtc;


/*!
    @brief  Convert a string of digits to an 8-bit unsigned integer.

    Negative numbers are not supported. Overflows are not handled.
    @param str  The string.
    @return The number.
            Returns 0 if the first character of the string is not a digit.
    @see struint16_
*/
byte strbyte_(const char* str)
{
    byte result = 0;
    while (isDigit(*str))
    {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}


/*!
    @brief  Find the first digit in a string.
    @param str  The string.
    @return Pointer to the digit.
*/
char* find_digit_(char* str)
{
    while (!isDigit(*str) && *str != '\0') str++;
    return str;
}



/*!
    @brief  Find the next number in a string.

    If the first character of the string is a digit, it skips all following
    characters until it finds one that is not a digit and then calls
    find_digit_.
    @param str  The string.
    @return Pointer to the first digit of the next number.
    @see find_digit_
*/
char* find_next_digit_(char* str)
{
    while (isDigit(*str) && *str != '\0') str++;
    str = find_digit_(str);
    return str;
}


void cmnd_rtc(char *args, commandResponse *response)
{
    (void) args;
    char buff[] = "YYYY-MM-DD hh:mm:ss";
    response->println(rtc.now().toString(buff));
}

void cmnd_sd(char *args, commandResponse *response)
{
    byte year, month, day;

    args = find_next_digit_(args) + 1; // +1 to avoid byte overflow
    if (*args == '\0') return;
    year = strbyte_(args);
    args = find_next_digit_(args);
    if (*args == '\0') return;
    month = strbyte_(args);
    args = find_next_digit_(args);
    if (*args == '\0') return;
    day = strbyte_(args);

    if (month > 12 || day > 31) return;

    DateTime now_ = rtc.now();
    DateTime new_date(year, month, day,
                      now_.hour(), now_.minute(), now_.second());
    if (!new_date.isValid()) return;
    rtc.adjust(new_date);
    response->println("ok");
}


void cmnd_st(char *args, commandResponse *response)
{
    byte hour, minute, second = 0;
    args = find_next_digit_(args);
    if (*args == '\0') return;
    hour = strbyte_(args);
    args = find_next_digit_(args);
    if (*args == '\0') return;
    minute = strbyte_(args);
    args = find_next_digit_(args);
    // Second is optional.
    if (*args != '\0')
        second = strbyte_(args);

    if (hour > 23 || minute > 59 || second > 59) return;

    DateTime now_ = rtc.now();
    rtc.adjust(DateTime(now_.year(), now_.month(), now_.day(), hour, minute, second));
    response->println("ok");
}


Commander::API_t API_tree[] = {
    apiElement("rtc",     "Get time.",     cmnd_rtc),
    apiElement("sd",      "Set date.",     cmnd_sd),
    apiElement("st",      "Set time.",     cmnd_st),
    // help -d to print out descriptions
};


void CLI_init()
{
  // Clear the terminal
  shell.clear();

  // Attach the logo.
  //shell.attachLogo( logo );

  // There is an option to attach a debug channel to Commander.
  // It can be handy to find any problems during the initialization
  // phase.
  //commander.attachDebugChannel( &Serial );

  commander.attachTree(API_tree);

  commander.init();

  shell.attachCommander(&commander);

  shell.begin("ele");
}


void CLI_loop()
{
    shell.update();
}
