#include "RTC_sample.h"

#include <cstdio>

#include "STM32RTC.h"

using namespace std;
using namespace codal;

void print_help();
void read_number(STM32Serial& serial, const char* prompt, uint16_t min_value, uint16_t max_value, uint16_t* value);

const char* weekdayToStr(uint8_t wd)
{
    switch (wd) {
        case 1:
            return "Mon.";
        case 2:
            return "Tue.";
        case 3:
            return "Wed.";
        case 4:
            return "Thu.";
        case 5:
            return "Fri.";
        case 6:
            return "Sat.";
        case 7:
            return "Sun.";
        default:
            return "???.";
    }
}

void RTC_main(STeaMi& steami)
{
    auto rtc     = STM32RTC();
    auto& serial = steami.serial;

    serial.init(115'200);

    printf("\r\n");
    printf("********************************************\r\n");
    printf("*            Demonstration du RTC          *\r\n");
    printf("********************************************\r\n");

    if (!rtc.init()) {
        printf("RTC init failed...\r\n");

        while (1) {
            steami.sleep(1);
        }
    }

    while (true) {
        int c = serial.read(ASYNC);
        switch (c) {
            case DEVICE_NO_DATA:
                break;

            case DEVICE_NO_RESOURCES:
                printf("[ERROR] : No ressources...\r\n");
                break;

            case 's':
            case 'S': {
                uint16_t hours, minutes, seconds;
                uint16_t weekday, day, month, year;
                read_number(serial, "Enter hour value (0-23):\r\n", 0, 23, &hours);
                read_number(serial, "Enter minutes value (0-59):\r\n", 0, 59, &minutes);
                read_number(serial, "Enter seconds value (0-59):\r\n", 0, 59, &seconds);

                printf("Set time to %02u:%02u:%02u .....[%s]\r\n", hours, minutes, seconds,
                       rtc.setTime(hours, minutes, seconds) ? "OK" : "FAIL");

                read_number(serial, "Enter day of the week value. 1 = Monday - 7 = Sunday (1-7):\r\n", 1, 7, &weekday);
                read_number(serial, "Enter day value (0-31):\r\n", 0, 31, &day);
                read_number(serial, "Enter month value (1-12):\r\n", 1, 12, &month);
                read_number(serial, "Enter year value (0-99):\r\n", 0, 99, &year);
                printf(
                    "Set date to %s %02u/%02u/%02u (dd/mm/yy) .....[%s]\r\n", weekdayToStr(weekday), day, month, year,
                    rtc.setDate(static_cast<RTC_Week_Day>(weekday), day, static_cast<RTC_Month>(month), year) ? "OK"
                                                                                                              : "FAIL");
                target_reset();
                break;
            }

            case 'g':
            case 'G': {
                auto time = rtc.getTime();
                auto date = rtc.getDate();

                printf("Read from RTC:\r\n");
                if (rtc.isHourFormat12())
                    printf("\tTime: %02u:%02u:%02u %s\r\n", time.hours, time.minutes, time.seconds,
                           time.amPm == RTC_Time_AM_PM::AM ? "AM" : "PM");
                else
                    printf("\tTime: %02u:%02u:%02u\r\n", time.hours, time.minutes, time.seconds);

                printf("\tDate (dd/mm/yy): %s %02u/%02u/%02u\r\n", weekdayToStr(static_cast<uint8_t>(date.weekday)),
                       date.day, static_cast<uint8_t>(date.month), date.year);
                break;
            }

            default:
                print_help();
                break;
        }
        steami.sleep(100);
    }
}

void print_help()
{
    printf("HELP\r\n");
    printf("\r\n");
    printf("\tS/s: Set the time & date\r\n");
    printf("\tG/g: Get the time & date\r\n");
    printf("\r\n");
}

void purge_serial_buffer(STM32Serial& serial)
{
    while (serial.getChar(SerialMode::ASYNC) != DEVICE_NO_DATA);
}

void read_number(STM32Serial& serial, const char* prompt, uint16_t min_value, uint16_t max_value, uint16_t* value)
{
    uint16_t tmp_value = max_value + 1;

    while (true) {
        purge_serial_buffer(serial);
        uint8_t buffer[2] = {0};
        printf(prompt);

        serial.read(buffer, 2, SerialMode::SYNC_SPINWAIT);
        sscanf((const char*)buffer, "%2u", &tmp_value);

        if (tmp_value < min_value || tmp_value > max_value) {
            printf("Bad value. the value must be within the range [%u; %u]\r\n", min_value, max_value);
        }
        else {
            break;
        }
    }

    *value = tmp_value;
}