#include <Arduino.h>
#include <RTClib.h>
#include <SD.h>
#include <TimerOne.h>

#define pin_SD_SS 10
#define pin_umount 7
#define pin_sensor 2

#define DATE_FORMAT "YYYY-MM-DD hh:mm:ss"

RTC_DS3231 rtc;

File log_file;

static volatile unsigned long pulses = 0;


#define SAMPLING_FREQUENCY 1000  // Hz
#define DEBOUNCE_TIME 8  // ms
#define INTEGRATOR_MAX (DEBOUNCE_TIME * SAMPLING_FREQUENCY / 1000)
static void read_sensor()
{
    static unsigned int integrator = 0;

    if (digitalRead(pin_sensor))
    {
        if (integrator < INTEGRATOR_MAX) integrator++;
    }
    else
    {
        if (integrator > 0) integrator--;
    }

    static bool output = false;
    static bool prev_output = false;
    static bool first = true;
    if (integrator == 0)
    {
        output = false;
        if (first)
        {
            first = false;
            prev_output = output;
        }
    }

    if (integrator >= INTEGRATOR_MAX)
    {
        output = true;
        if (first)
        {
            first = false;
            prev_output = output;
        }
        // this should never be needed
        integrator = INTEGRATOR_MAX;
    }

    if (output != prev_output)
    {
        prev_output = output;
        if (output) pulses++;
    }
}


void write_log(DateTime now, unsigned long sensor_pulses)
{
    char time_str[] = DATE_FORMAT "\t";
    now.toString(time_str);

    if (log_file)
    {
        log_file.print(time_str);
        log_file.print(sensor_pulses);
        log_file.print('\t');
        log_file.print(rtc.getTemperature());
        log_file.print('\n');
    }
}


void setup()
{
    pinMode(pin_umount, INPUT_PULLUP);
    pinMode(pin_sensor, INPUT);

    Serial.begin(115200);
    Serial.println(F("elektromer-logger"));

    delay(500);  // wait for power to stabilize

    if (!rtc.begin())
    {
        for (;;)
        {
            Serial.println(F("RTC init error"));
            delay(200);
        }
    }

    if (rtc.lostPower())
    {
        Serial.println(F("RTC lost power (need to set time)"));
        Serial.println(F("Setting time " __DATE__ " " __TIME__));
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    DateTime time = rtc.now();
    char time_str[] = DATE_FORMAT;
    time.toString(time_str);
    Serial.print(F("RTC time: "));
    Serial.println(time_str);

    if (!SD.begin(pin_SD_SS))
    {
        for (;;)
        {
            Serial.println(F("SD card init error"));
            delay(200);
        }
    }

    log_file = SD.open("elektro.log", FILE_WRITE);  // this should append
    log_file.print(F("--------\n"));

    Timer1.initialize(1000000UL / SAMPLING_FREQUENCY);
    Timer1.attachInterrupt(read_sensor);
}


void loop()
{
    static unsigned long sensor_pulses = 0;
    if (!digitalRead(pin_umount))
    {
        DateTime time = rtc.now();
        write_log(time, sensor_pulses);

        if (log_file) log_file.close();

        for (;;)
        {
            Serial.println(F("SD card umounted"));
            delay(200);
        }
    }

    unsigned long now = millis();
    static unsigned long prev_print = 0;
    if (now - prev_print >= 1000UL)
    {
        prev_print = now;
        noInterrupts();
        sensor_pulses = pulses;
        interrupts();
        Serial.print(F("pulses: "));
        Serial.println(sensor_pulses);
    }

    static unsigned long prev = 0;
#define WRITE_CHECK_INTERVAL 500UL
    static unsigned long write_check_interval = WRITE_CHECK_INTERVAL;
    if (now - prev >= write_check_interval)
    {
        prev = now;
        write_check_interval = WRITE_CHECK_INTERVAL;
        DateTime time = rtc.now();
        if (time.minute() % 5 == 0 && time.second() == 0)
        {
            write_check_interval = 1500UL;  // do not trigger again during this second
            noInterrupts();
            sensor_pulses = pulses;
            interrupts();
            write_log(time, sensor_pulses);
        }
    }
}
