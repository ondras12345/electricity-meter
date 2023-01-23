#include <Arduino.h>
#include <RTClib.h>
#include <SD.h>
#include <TimerOne.h>

#define pin_SD_SS 10
#define pin_umount 7
#define pin_sensor 2

#define DATE_FORMAT "YYYY-MM-DD hh:mm:ss"
#define LOG_INTERVAL_MINUTES 1
#define LOG_FILENAME "elektro.log"

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


void conf_time()
{
    Serial.println(F("RTC config - set UTC"));
    unsigned int year;
    do
    {
        while (Serial.available()) Serial.read();
        Serial.println(F("RTC year: "));
        while (!Serial.available());
        year = Serial.parseInt();
    } while (year < 2000 || year > 2099);

    unsigned int month;
    do
    {
        while (Serial.available()) Serial.read();
        Serial.println(F("RTC month: "));
        while (!Serial.available());
        month = Serial.parseInt();
    } while (month < 1 || month > 12);

    unsigned int day;
    do
    {
        while (Serial.available()) Serial.read();
        Serial.println(F("RTC day: "));
        while (!Serial.available());
        day = Serial.parseInt();
    } while (day < 1 || day > 31);

    unsigned int hour;
    do
    {
        while (Serial.available()) Serial.read();
        Serial.println(F("RTC hour: "));
        while (!Serial.available());
        hour = Serial.parseInt();
    } while (hour >= 24);

    unsigned int minute;
    do
    {
        while (Serial.available()) Serial.read();
        Serial.println(F("RTC minute: "));
        while (!Serial.available());
        minute = Serial.parseInt();
    } while (minute >= 60);

    unsigned int second;
    do
    {
        while (Serial.available()) Serial.read();
        Serial.println(F("RTC second: "));
        while (!Serial.available());
        second = Serial.parseInt();
    } while (second >= 60);

    DateTime new_dt(year, month, day, hour, minute, second);
    if (!new_dt.isValid())
    {
        Serial.println(F("Bad datetime, ignored"));
        return;
    }

    rtc.adjust(new_dt);
    Serial.println(F("datetime set"));
}


void conf_file()
{
    while (Serial.available()) Serial.read();

    for (;;)
    {
        do
        {
            Serial.println(F("'d' dump, 'r' rm file, 'x' exit"));
            delay(500);
        } while (!Serial.available());

        switch (Serial.read())
        {
            case 'd':
                {
                    File f = SD.open(LOG_FILENAME);
                    if (!f)
                    {
                        Serial.println(F("file not found"));
                        break;
                    }
                    Serial.println(F("--- BEGIN LOG FILE ---"));
                    while (f.available())
                    {
                        char c = f.read();
                        if (c == '\n') Serial.write('\r');
                        Serial.write(c);
                    }
                    Serial.println(F("--- END LOG FILE ---"));
                    f.close();
                }
                break;

            case 'r':
                Serial.println(F("Press button to confirm file rm"));
                while (digitalRead(pin_umount));
                SD.remove(LOG_FILENAME);
                break;

            case 'x':
                return;
        }
    }
}


void setup()
{
    pinMode(pin_umount, INPUT_PULLUP);
    pinMode(pin_sensor, INPUT);

    Serial.begin(115200);
    Serial.println(F("elektromer-logger"));
    Serial.println(F("press c to enter config"));

    delay(500);  // wait for power to stabilize

    bool config = false;
    while (Serial.available())
    {
        if (Serial.read() == 'c')
        {
            config = true;
        }

    }

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
    Serial.print(F("RTC time (UTC): "));
    Serial.println(time_str);

    bool config_file = false;
    if (config)
    {
        while (Serial.available()) Serial.read();
        bool valid = false;
        while (!valid)
        {
            if (!Serial.available())
            {
                do
                {
                    Serial.println(F("config: 't' time, 'f' file, 'x' exit"));
                    delay(500);
                }
                while (!Serial.available());
            }
            switch (Serial.read())
            {
                case 't':
                    conf_time();
                    valid = true;
                    break;

                case 'f':
                    config_file = true;
                    valid = true;
                    break;

                case 'x':
                    valid = true;
                    break;
            }
        }

        config = false;
    }

    if (!SD.begin(pin_SD_SS))
    {
        for (;;)
        {
            Serial.println(F("SD card init error"));
            delay(200);
        }
    }

    if (config_file)
    {
        conf_file();
        config_file = false;
    }

    log_file = SD.open(LOG_FILENAME, FILE_WRITE);  // this should append
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
        if (time.minute() % LOG_INTERVAL_MINUTES == 0 && time.second() == 0)
        {
            write_check_interval = 1500UL;  // do not trigger again during this second
            noInterrupts();
            sensor_pulses = pulses;
            interrupts();
            write_log(time, sensor_pulses);
        }
    }
}
