#include "demo_sample.h"

using namespace codal;
using namespace std;

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include "APDS9960.h"
#include "BQ27441.h"
#include "HTS221.h"
#include "STM32Pin.h"
#include "STM32RTC.h"
#include "STM32SAI.h"
#include "STM32SingleWireSerial.h"
#include "VL53L1X.h"
#include "WSEN-PADS.h"
#include "daplink_flash.h"
#include "ism330dl.h"
#include "lis2mdl.h"
#include "mcp23009-e.h"
#include "menu.h"
#include "pcm_utils.h"
#include "pdm2pcm.h"
#include "ssd1327.h"

constexpr uint16_t AUDIO_BUFFER = 256;

constexpr uint8_t MCP_GPIO_1    = 0;
constexpr uint8_t MCP_GPIO_2    = 1;
constexpr uint8_t MCP_GPIO_3    = 2;
constexpr uint8_t MCP_GPIO_4    = 3;
constexpr uint8_t MCP_GP_RIGHT  = 4;
constexpr uint8_t MCP_GP_BOTTOM = 5;
constexpr uint8_t MCP_GP_LEFT   = 6;
constexpr uint8_t MCP_GP_UP     = 7;

STM32I2C* i2c_qwiic             = nullptr;
STM32SPI* spi                   = nullptr;
STM32Pin* cs                    = nullptr;
STM32Pin* dc                    = nullptr;
STM32Pin* rst                   = nullptr;
STM32Pin* led_red               = nullptr;
STM32Pin* led_green             = nullptr;
STM32Pin* led_blue              = nullptr;
SSD1327_SPI* ssd                = nullptr;

STM32Pin* btnMenu               = nullptr;
STM32Pin* btnA                  = nullptr;
STM32Pin* btnB                  = nullptr;
STM32Pin* buzzer                = nullptr;

BQ27441* battery                = nullptr;
MCP23009E* mcp                  = nullptr;
HTS221* hts                     = nullptr;
WSEN_PADS* pres                 = nullptr;
VL53L1X* tof                    = nullptr;
STM32SAI* sai                   = nullptr;
PDM2PCM pdm2pcm(16, 8, 0, 1);

APDS9960* apds = nullptr;
ISM330DL* ism  = nullptr;
LIS2MDL* lis   = nullptr;
STM32SingleWireSerial* jacdac;
DaplinkFlash* flash;

STM32RTC* rtc                         = nullptr;

ScreenMenu* mainMenu                  = nullptr;

STM32Pin* microbit_pins[19]           = {nullptr};
STM32Pin* microbit_pwm_pins[3]        = {nullptr};
STM32Pin* microbit_analog_pad_pins[3] = {nullptr};

uint16_t rawMicData[AUDIO_BUFFER];
bool processedMicData = true;

string fToStr(float value, unsigned pres)
{
    int ent = (int)value;
    int dec = (int)((value - ent) * pow(10, pres));

    return to_string(ent) + "." + to_string(abs(dec));
}

bool click_button(STM32Pin* btn)
{
    if (btn->getDigitalValue() == 0) {
        while (btn->getDigitalValue() == 0);

        return true;
    }

    return false;
}

void show_main_menu()
{
    mcp->interruptOnFalling(MCP_GP_BOTTOM, []() { mainMenu->moveDown(); });
    mcp->interruptOnFalling(MCP_GP_UP, []() { mainMenu->moveUp(); });

    mainMenu->show();

    while (1) {
        if (btnA->getDigitalValue() == 0) {
            while (btnA->getDigitalValue() == 0);
            break;
        }

        fiber_sleep(1);
    }

    mcp->disableInterrupt(MCP_GP_BOTTOM);
    mcp->disableInterrupt(MCP_GP_UP);
    mainMenu->execute();
}

void show_temp_hum()
{
    while (1) {
        fiber_sleep(125);

        if (click_button(btnMenu)) {
            break;
        }

        if (!hts->isTemperatureDataAvailable() || !hts->isHumidityDataAvailable()) {
            printf("HTS221 not ready\r\n");
            continue;
        }

        ssd->fill(0x00);
        ssd->drawText("Temperature: " + fToStr(hts->getTemperature(), 2) + " C", 5, 55, 0xFF);
        ssd->drawText("Humidite: " + fToStr(hts->getHumidity(), 2) + " %HR", 5, 65, 0xFF);

        ssd->show();
    }
}

void show_pressure()
{
    while (1) {
        fiber_sleep(125);

        if (click_button(btnMenu)) {
            break;
        }

        ssd->fill(0x00);
        ssd->drawText("Pression: " + fToStr(pres->getPressure(), 2) + " kPa", 5, 55, 0xFF);
        ssd->drawText("Temperature: " + fToStr(pres->getTemperature(), 2) + " C", 5, 65, 0xFF);

        ssd->show();
    }
}

void show_tof()
{
    while (1) {
        fiber_sleep(100);

        if (click_button(btnMenu)) {
            break;
        }

        ssd->fill(0x00);
        ssd->drawText("Distance: " + fToStr(tof->getDistance(), 2) + " mm", 5, 60, 0xFF);
        ssd->show();
    }
}

void show_acc_gyro_magn()
{
    int8_t selection = 0;

    while (1) {
        if (click_button(btnMenu)) {
            break;
        }

        if (mcp->getLevel(MCP_GP_LEFT) == MCP_LOGIC_LEVEL::LOW) {
            while (mcp->getLevel(MCP_GP_LEFT) == MCP_LOGIC_LEVEL::LOW);

            selection--;
        }

        if (mcp->getLevel(MCP_GP_RIGHT) == MCP_LOGIC_LEVEL::LOW) {
            while (mcp->getLevel(MCP_GP_RIGHT) == MCP_LOGIC_LEVEL::LOW);

            selection++;
        }

        ssd->fill(0x00);

        if (selection == 0) {
            ssd->drawText("Accelerometer", 25, 20, 0xFF);
            auto accel = ism->readAccelerometerData();

            ssd->drawText("X: " + fToStr(accel.x, 2), 20, 49, 0xFF);
            ssd->drawText("Y: " + fToStr(accel.y, 2), 20, 58, 0xFF);
            ssd->drawText("Z: " + fToStr(accel.z, 2), 20, 67, 0xFF);

            ssd->drawText("<- Magneto", 20, 87, 0xFF);
            ssd->drawText("-> Gyro", 20, 96, 0xFF);
        }
        else if (selection == 1) {
            ssd->drawText("Gyroscope", 30, 20, 0xFF);
            auto gyro = ism->readGyroscopeData();

            ssd->drawText("X: " + fToStr(gyro.x, 2), 20, 49, 0xFF);
            ssd->drawText("Y: " + fToStr(gyro.y, 2), 20, 58, 0xFF);
            ssd->drawText("Z: " + fToStr(gyro.z, 2), 20, 67, 0xFF);

            ssd->drawText("<- Accelero", 20, 87, 0xFF);
            ssd->drawText("-> Magneto", 20, 96, 0xFF);
        }
        else if (selection == 2) {
            ssd->drawText("Magnetometer", 25, 20, 0xFF);

            auto magn = lis->readData();

            ssd->drawText("X: " + fToStr(magn.x, 2), 20, 49, 0xFF);
            ssd->drawText("Y: " + fToStr(magn.y, 2), 20, 58, 0xFF);
            ssd->drawText("Z: " + fToStr(magn.z, 2), 20, 67, 0xFF);

            ssd->drawText("<- Gyro", 20, 87, 0xFF);
            ssd->drawText("-> Accelero", 20, 96, 0xFF);
        }
        else {
            selection = selection < 0 ? 2 : 0;
            continue;
        }

        ssd->show();
        fiber_sleep(125);
    }
}

void show_optical_sensor()
{
    while (1) {
        fiber_sleep(100);

        if (click_button(btnMenu)) {
            break;
        }

        std::array<uint16_t, 4> color_data = apds->getColors();

        ssd->fill(0x00);
        ssd->drawText("Color Sensor", 25, 20, 0xFF);
        ssd->drawText("(untested)", 25, 28, 0xFF);
        ssd->drawText("Red  : " + to_string(color_data[0]), 20, 49, 0xFF);
        ssd->drawText("Green: " + to_string(color_data[1]), 20, 58, 0xFF);
        ssd->drawText("Blue : " + to_string(color_data[2]), 20, 67, 0xFF);
        ssd->drawText("Clear: " + to_string(color_data[3]), 20, 76, 0xFF);
        ssd->show();
    }
}

void show_microphone()
{
    sai->startListening();

    while (1) {
        fiber_sleep(100);

        if (click_button(btnMenu)) {
            break;
        }

        if (!processedMicData) {
            auto conv = pdm2pcm.convert(rawMicData, AUDIO_BUFFER);

            float avg = 0;
            for (auto v : conv) {
                avg += float(v) / float(conv.size());
            }

            ssd->fill(0x00);
            ssd->drawText("Decibel: " + fToStr(PCMUtils::toDecibel(conv), 1) + "dB", 5, 60, 0xFF);
            ssd->show();

            processedMicData = true;
        }
    }

    sai->stopListening();
}

void micro_on_data(const int32_t* data)
{
    if (processedMicData) {
        for (uint16_t i = 0; i < AUDIO_BUFFER; ++i) {
            rawMicData[i] = uint16_t(data[i] & 0x0000FFFF);
        }

        processedMicData = false;
    }
}

void show_buzzer()
{
    uint8_t* cursor = new uint8_t(2);
    int* freq       = new int(440);

    mcp->interruptOnFalling(MCP_GP_BOTTOM, [=]() {
        int p     = pow(10, *cursor);
        int digit = int(*freq / p) % 10;
        if (digit == 0) {
            return;
        }

        *freq -= p;
        buzzer->setAnalogPeriodUs(1'000'000 / *freq);
        buzzer->setAnalogValue(255);
    });

    mcp->interruptOnFalling(MCP_GP_UP, [=]() {
        int p     = pow(10, *cursor);
        int digit = int(*freq / p) % 10;
        if (digit == 9) {
            return;
        }

        *freq += p;
        buzzer->setAnalogPeriodUs(1'000'000 / *freq);
        buzzer->setAnalogValue(255);
    });

    mcp->interruptOnFalling(MCP_GP_LEFT, [=]() {
        if (*cursor < 5) {
            (*cursor)++;
        }
    });

    mcp->interruptOnFalling(MCP_GP_RIGHT, [=]() {
        if (*cursor > 0) {
            (*cursor)--;
        }
    });

    buzzer->setAnalogPeriodUs(1'000'000 / *freq);
    buzzer->setAnalogValue(255);

    while (1) {
        if (click_button(btnMenu)) {
            break;
        }

        auto freqStr     = std::to_string(*freq);
        string cursorStr = "      ";

        while (freqStr.size() < 5) {
            freqStr = "0" + freqStr;
        }

        for (int8_t i = 4; i >= 0; --i) {
            if (i == *cursor) {
                cursorStr.push_back('^');
            }
            else {
                cursorStr.push_back(' ');
            }
        }

        ssd->fill(0x00);
        ssd->drawText("Freq: " + freqStr + "Hz", 15, 40, 0xFF);
        ssd->drawText(cursorStr, 15, 48, 0xFF);
        ssd->drawText("< / >: change digit", 5, 66, 0xFF);
        ssd->drawText("^ / v: de/increase", 5, 75, 0xFF);
        ssd->show();

        fiber_sleep(100);
    }

    buzzer->setAnalogValue(0);
    mcp->disableInterrupt(MCP_GP_BOTTOM);
    mcp->disableInterrupt(MCP_GP_UP);
    delete cursor;
    delete freq;
}

const char* weekdayToStr(uint8_t wd)
{
    switch (wd) {
        case 1:
            return "Monday";
        case 2:
            return "Tuesday";
        case 3:
            return "Wednesday";
        case 4:
            return "Thursday";
        case 5:
            return "Friday";
        case 6:
            return "Saturday";
        case 7:
            return "Sunday";
        default:
            return "???";
    }
}

void inc_limit(uint8_t* value, uint8_t min, uint8_t max)
{
    if (*value == max) {
        *value = min;
    }
    else {
        (*value)++;
    }
}

void dec_limit(uint8_t* value, uint8_t min, uint8_t max)
{
    if (*value == min) {
        *value = max;
    }
    else {
        (*value)--;
    }
}

void show_rtc()
{
    uint8_t* page         = new uint8_t{0};
    uint8_t* weekday      = new uint8_t{0};
    uint8_t* day          = new uint8_t{0};
    uint8_t* month        = new uint8_t{0};
    uint8_t* year         = new uint8_t{0};
    uint8_t* hours        = new uint8_t{0};
    uint8_t* minutes      = new uint8_t{0};
    uint8_t* seconds      = new uint8_t{0};
    bool* isInSettingMode = new bool{false};
    char time_str[20]     = {0};
    char date_str[20]     = {0};
    char setting_str[20]  = {0};

    RTC_Date date;
    RTC_Time time;

    mcp->interruptOnFalling(MCP_GP_BOTTOM, [=]() {
        if (!isInSettingMode) return;

        switch (*page) {
            case 0:
                dec_limit(weekday, 1, 7);
                break;

            case 1:
                dec_limit(day, 1, 31);
                break;

            case 2:
                dec_limit(month, 1, 12);
                break;

            case 3:
                dec_limit(year, 0, 99);
                break;

            case 4:
                dec_limit(hours, 0, 23);
                break;

            case 5:
                dec_limit(minutes, 0, 59);
                break;

            case 6:
                dec_limit(seconds, 0, 59);
                break;

            default:
                break;
        }
    });

    mcp->interruptOnFalling(MCP_GP_UP, [=]() {
        if (!isInSettingMode) return;

        switch (*page) {
            case 0:
                inc_limit(weekday, 1, 7);
                break;

            case 1:
                inc_limit(day, 1, 31);
                break;

            case 2:
                inc_limit(month, 1, 12);
                break;

            case 3:
                inc_limit(year, 0, 99);
                break;

            case 4:
                inc_limit(hours, 1, 23);
                break;

            case 5:
                inc_limit(minutes, 0, 59);
                break;

            case 6:
                inc_limit(seconds, 0, 59);
                break;

            default:
                break;
        }
    });

    mcp->interruptOnFalling(MCP_GP_LEFT, [=]() { dec_limit(page, 0, 6); });

    mcp->interruptOnFalling(MCP_GP_RIGHT, [=]() { inc_limit(page, 0, 6); });

    while (1) {
        if (click_button(btnMenu)) {
            break;
        }

        if (*isInSettingMode) {
            ssd->fill(0x00);

            switch (*page) {
                case 0:
                    sprintf(setting_str, "   %s", weekdayToStr(*weekday));
                    break;

                case 1:
                    sprintf(setting_str, "  Day: %02u", *day);
                    break;

                case 2:
                    sprintf(setting_str, " Month: %02u", *month);
                    break;

                case 3:
                    sprintf(setting_str, "   Year: %02u", *year);
                    break;

                case 4:
                    sprintf(setting_str, "  Hours: %02u", *hours);
                    break;

                case 5:
                    sprintf(setting_str, "Minutes: %02u", *minutes);
                    break;

                case 6:
                    sprintf(setting_str, "Seconds: %02u", *seconds);
                    break;

                default:
                    break;
            }

            ssd->drawText(setting_str, 15, 40, 0xFF);
            ssd->drawText("^ / v: add / sub", 10, 76, 0xFF);
            ssd->drawText("< / >: next / prev", 5, 86, 0xFF);
            ssd->drawText("A : Save & return", 13, 96, 0xFF);
            ssd->drawText("B : Cancel", 23, 106, 0xFF);
            ssd->show();

            if (click_button(btnA)) {
                *isInSettingMode = false;
                *page            = 0;
                rtc->setTime(*hours, *minutes, *seconds);
                rtc->setDate(static_cast<RTC_Week_Day>(*weekday), *day, static_cast<RTC_Month>(*month), *year);
            }
            if (click_button(btnB)) {
                *isInSettingMode = false;
                *page            = 0;
            }
        }
        else {
            date     = rtc->getDate();
            time     = rtc->getTime();

            *weekday = static_cast<uint8_t>(date.weekday);
            *day     = date.day;
            *month   = static_cast<uint8_t>(date.month);
            *year    = date.year;
            *hours   = time.hours;
            *minutes = time.minutes;
            *seconds = time.seconds;

            sprintf(date_str, "%.3s. %02u/%02u/%02u", weekdayToStr(*weekday), *day, *month, *year);
            sprintf(time_str, "%02u:%02u:%02u", *hours, *minutes, *seconds);

            ssd->fill(0x00);
            ssd->drawText(date_str, 25, 30, 0xFF);
            ssd->drawText(time_str, 40, 40, 0xFF);
            ssd->drawText("A : Set date & time", 8, 75, 0xFF);
            ssd->show();

            if (click_button(btnA)) {
                *isInSettingMode = true;
            }
        }

        fiber_sleep(100);
    }
}

void show_button()
{
    while (1) {
        if (click_button(btnMenu)) {
            break;
        }

        bool up_state    = mcp->getLevel(MCP_GP_UP) == MCP_LOGIC_LEVEL::HIGH;
        bool down_state  = mcp->getLevel(MCP_GP_BOTTOM) == MCP_LOGIC_LEVEL::HIGH;
        bool left_state  = mcp->getLevel(MCP_GP_LEFT) == MCP_LOGIC_LEVEL::HIGH;
        bool right_state = mcp->getLevel(MCP_GP_RIGHT) == MCP_LOGIC_LEVEL::HIGH;
        bool a_state     = btnA->getDigitalValue() == 1;
        bool b_state     = btnB->getDigitalValue() == 1;

        ssd->fill(0x00);
        ssd->drawText("Up: " + string(up_state ? "HIGH" : "LOW"), 12, 33, 0xFF);
        ssd->drawText("Down: " + string(down_state ? "HIGH" : "LOW"), 7, 45, 0xFF);
        ssd->drawText("Left: " + string(left_state ? "HIGH" : "LOW"), 2, 56, 0xFF);
        ssd->drawText("Right: " + string(right_state ? "HIGH" : "LOW"), 2, 69, 0xFF);
        ssd->drawText("A: " + string(a_state ? "HIGH" : "LOW"), 7, 81, 0xFF);
        ssd->drawText("B: " + string(b_state ? "HIGH" : "LOW"), 12, 93, 0xFF);
        ssd->show();

        fiber_sleep(1);
    }
}

void show_rgb()
{
    uint8_t* select_line = new uint8_t(0);
    int state_red        = 0;
    int state_green      = 0;
    int state_blue       = 0;

    mcp->interruptOnFalling(MCP_GP_BOTTOM, [=]() {
        if (*select_line < 2) {
            (*select_line)++;
        }
    });

    mcp->interruptOnFalling(MCP_GP_UP, [=]() {
        if (*select_line > 0) {
            (*select_line)--;
        }
    });

    while (1) {
        if (click_button(btnMenu)) {
            break;
        }

        if (click_button(btnA)) {
            switch (*select_line) {
                case 0:
                    state_red = state_red == 0 ? 1 : 0;
                    led_red->setDigitalValue(state_red);
                    break;

                case 1:
                    state_green = state_green == 0 ? 1 : 0;
                    led_green->setDigitalValue(state_green);
                    break;

                case 2:
                    state_blue = state_blue == 0 ? 1 : 0;
                    led_blue->setDigitalValue(state_blue);
                    break;
            }
        }

        ssd->fill(0x00);
        ssd->drawText("Toggle Red LED", 15, 51, 0xFF);
        ssd->drawText("Toggle Green LED", 15, 60, 0xFF);
        ssd->drawText("Toggle Blue LED", 15, 69, 0xFF);
        ssd->drawText(">", 5, (*select_line) * 9 + 51, 0xFF);
        ssd->show();

        fiber_sleep(1);
    }

    mcp->disableInterrupt(MCP_GP_BOTTOM);
    mcp->disableInterrupt(MCP_GP_UP);
    led_red->setDigitalValue(0);
    led_green->setDigitalValue(0);
    led_blue->setDigitalValue(0);
    delete select_line;
}

void show_screen()
{
    ssd->fill(0x00);

    uint16_t color = 0xFF;
    uint8_t shape  = 4;

    while (1) {
        for (unsigned i = 0; i < 35; ++i) {
            if (click_button(btnMenu)) {
                return;
            }

            if (shape == 0)
                ssd->drawCircle(64, 64, i, true, color);
            else if (shape == 1)
                ssd->drawRectangle(64 - i * 2, 64 - i * 2, 64 + i * 2, 64 + i * 2, true, color);
            else if (shape == 2)
                ssd->drawPolygon(64, 64, 5, i * 2, 3, color);
            else if (shape == 3)
                ssd->drawPolygon(64, 64, 3, i * 2, 3, color);
            else if (shape == 4)
                for (float deg = 0; deg < 360; deg += 0.5) {
                    float theta = deg * 0.0174533;
                    float r     = 10 * theta;  // pow(5, theta);

                    ssd->drawPixel(r * cos(r) + 64, r * sin(r) + 64, color);
                }

            ssd->show();
        }

        color = ~color;
        shape++;
        if (shape >= 5) shape = 0;
    }
}

void show_battery()
{
    uint32_t timeout   = getCurrentMillis();
    bool is_first_page = true;
    char buff[16]      = {0};

    battery->reset();

    ssd->fill(0x00);
    ssd->drawText("Waiting for the init", 4, 60, 0xFF);
    ssd->show();

    while (1) {
        if (click_button(btnMenu)) {
            break;
        }

        if (!battery->is_init()) {
            if ((getCurrentMillis() - timeout) >= 5'000) {
                ssd->fill(0x00);
                ssd->drawText("TIMEOUT", 44, 50, 0xFF);
                ssd->drawText("No battery connected", 4, 60, 0xFF);
                ssd->show();
            }
        }
        else {
            if (is_first_page) {
                ssd->fill(0x00);
                ssd->drawText("Charge: " + fToStr(battery->state_of_charge(), 0) + "%", 5, 45, 0xFF);
                ssd->drawText("Voltage: " + fToStr(battery->get_voltage(), 2) + "V", 5, 53, 0xFF);
                ssd->drawText("Avg. Current: " + fToStr(battery->get_average_current(), 2) + "A", 5, 62, 0xFF);
                ssd->drawText("Avg. Power: " + fToStr(battery->get_average_power(), 2) + "W", 5, 71, 0xFF);
                ssd->show();
            }
            else {
                ssd->fill(0x00);
                ssd->drawText("Temp.: " + fToStr(battery->get_temperature(), 2) + "C", 5, 45, 0xFF);

                sprintf(buff, "0x%04X", battery->device_type());
                ssd->drawText("Dev. type: " + std::string(buff), 5, 53, 0xFF);

                sprintf(buff, "0x%04X", battery->firmware_version());
                ssd->drawText("Firm. ver: " + std::string(buff), 5, 62, 0xFF);

                sprintf(buff, "0x%04X", battery->dm_code());
                ssd->drawText("DM code: " + std::string(buff), 5, 71, 0xFF);

                ssd->show();
            }

            if (getCurrentMillis() - timeout > 3'000) {
                is_first_page = !is_first_page;
                timeout       = getCurrentMillis();
            }
        }

        fiber_sleep(10);
    }
}

void show_pads()
{
    int r = 3;

    while (1) {
        if (click_button(btnMenu)) {
            break;
        }

        bool gp1 = mcp->getLevel(MCP_GPIO_1) == MCP_LOGIC_LEVEL::HIGH;
        bool gp2 = mcp->getLevel(MCP_GPIO_2) == MCP_LOGIC_LEVEL::HIGH;
        bool gp3 = mcp->getLevel(MCP_GPIO_3) == MCP_LOGIC_LEVEL::HIGH;
        bool gp4 = mcp->getLevel(MCP_GPIO_4) == MCP_LOGIC_LEVEL::HIGH;

        ssd->fill(0x00);
        ssd->drawCircle(8, 41, r, true, 0xFF);
        ssd->drawCircle(14, 30, r, gp3, 0xFF);
        ssd->drawCircle(22, 20, r, gp4, 0xFF);
        ssd->drawCircle(32, 13, r, false, 0xFF);
        ssd->drawChar('G', 30, 10, 0xFF);
        ssd->drawChar('V', 6, 38, 0x00);

        ssd->drawCircle(119, 41, r, true, 0xFF);
        ssd->drawCircle(113, 30, r, gp2, 0xFF);
        ssd->drawCircle(105, 20, r, gp1, 0xFF);
        ssd->drawCircle(95, 13, r, false, 0xFF);
        ssd->drawChar('G', 93, 10, 0xFF);
        ssd->drawChar('V', 117, 38, 0x00);

        ssd->drawChar(gp3 ? '1' : '0', 12, 27, gp3 ? 0x00 : 0xFF);
        ssd->drawChar(gp4 ? '1' : '0', 20, 17, gp4 ? 0x00 : 0xFF);
        ssd->drawChar(gp2 ? '1' : '0', 111, 27, gp2 ? 0x00 : 0xFF);
        ssd->drawChar(gp1 ? '1' : '0', 103, 17, gp1 ? 0x00 : 0xFF);

        ssd->show();

        fiber_sleep(10);
    }
}

void show_microbit()
{
    constexpr uint16_t arc_angle = 360 / 19;
    uint8_t pin_index            = 0;
    uint32_t start_time          = 0;
    bool* is_led_mode            = new bool{true};

    mcp->interruptOnFalling(MCP_GP_BOTTOM, [=]() {
        *is_led_mode = !(*is_led_mode);

        if (*is_led_mode) {
            microbit_pins[0]->setDigitalValue(0);
        }
        else {
            microbit_pins[0]->getDigitalValue();
        }
    });

    for (int i = 0; i < 19; ++i) {
        microbit_pins[i]->setDigitalValue(0);
    }

    while (1) {
        if (click_button(btnMenu)) {
            break;
        }

        if (*is_led_mode) {
            if (getCurrentMillis() - start_time < 500) {
                continue;
            }

            if (pin_index == 0) {
                microbit_pins[18]->setDigitalValue(0);
            }
            else {
                microbit_pins[pin_index - 1]->setDigitalValue(0);
            }

            microbit_pins[pin_index]->setDigitalValue(1);

            ssd->fill(0x00);
            ssd->drawArc(64, 64, 60, arc_angle * pin_index, arc_angle * (pin_index + 1), 0xFF);
            ssd->drawText("Turn on pin:", 29, 40, 0xFF);
            ssd->drawText((pin_index + 1 < 10 ? "0" : "") + to_string(pin_index + 1), 57, 49, 0xFF);
            ssd->drawText("Down: Self test", 20, 90, 0xFF);
            ssd->show();

            pin_index++;
            if (pin_index >= 19) {
                pin_index = 0;
            }
            start_time = getCurrentMillis();
        }
        else {
            if (pin_index == 0) {
                microbit_pins[18]->setDigitalValue(0);
                pin_index = 1;
            }
            else {
                microbit_pins[pin_index - 1]->setDigitalValue(0);
            }

            microbit_pins[pin_index]->setDigitalValue(1);

            ssd->fill(0x00);
            ssd->drawArc(64, 64, 60, arc_angle * pin_index, arc_angle * (pin_index + 1), 0xFF);
            ssd->drawText("Turn on pin:", 29, 30, 0xFF);
            ssd->drawText((pin_index + 1 < 10 ? "0" : "") + to_string(pin_index + 1), 57, 39, 0xFF);
            ssd->drawText("Result:", 46, 57, 0xFF);
            ssd->drawText(microbit_pins[0]->getDigitalValue() == 1 ? "O.K" : "K.O", 55, 69, 0xFF);
            ssd->drawText("Down: LED test", 22, 90, 0xFF);
            ssd->show();

            if (getCurrentMillis() - start_time >= 500) {
                pin_index++;
                if (pin_index >= 19) {
                    pin_index = 0;
                }
                start_time = getCurrentMillis();
            }
        }

        fiber_sleep(1);
    }

    for (int i = 0; i < 19; ++i) {
        microbit_pins[i]->setDigitalValue(0);
    }

    btnA->getDigitalValue();
    btnB->getDigitalValue();
}

void show_pwm_microbit()
{
    constexpr uint16_t pwm_step = 20;
    uint32_t start_timeout      = 0;
    uint8_t* pin_id             = new uint8_t{0};
    uint16_t* pwm_value         = new uint16_t{0};

    mcp->interruptOnFalling(MCP_GP_BOTTOM, [=]() {
        *pin_id = ((*pin_id) == 2) ? 0 : (*pin_id) + 1;

        if (*pin_id == 0) {
            microbit_pwm_pins[2]->setAnalogValue(0);
        }
        else {
            microbit_pwm_pins[(*pin_id) - 1]->setAnalogValue(0);
        }

        (*pwm_value) = 0;
    });

    mcp->interruptOnFalling(MCP_GP_UP, [=]() {
        *pin_id = ((*pin_id) == 0) ? 2 : (*pin_id) - 1;

        if (*pin_id == 2) {
            microbit_pwm_pins[0]->setAnalogValue(0);
        }
        else {
            microbit_pwm_pins[(*pin_id) + 1]->setAnalogValue(0);
        }

        (*pwm_value) = 0;
    });

    for (int i = 0; i < 19; ++i) {
        microbit_pins[i]->setDigitalValue(0);
    }

    fiber_sleep(500);

    for (uint8_t i = 0; i < 3; ++i) {
        printf("Init PWM pin #%u (result: %d)\n", i, microbit_pwm_pins[i]->setAnalogValue(0));
    }

    while (1) {
        if (click_button(btnMenu)) {
            break;
        }

        if (getCurrentMillis() - start_timeout >= 100) {
            (*pwm_value) += pwm_step;

            if (*pwm_value >= 1'024) {
                *pwm_value = 0;
            }

            microbit_pwm_pins[*pin_id]->setAnalogValue(*pwm_value);

            ssd->fill(0x00);
            ssd->drawText("Pin: 0" + to_string(*pin_id), 42, 36, 0xFF);
            ssd->drawText(string("PWM value:") + ((*pwm_value < 1'000) ? "0" : "") + ((*pwm_value < 100) ? "0" : "") +
                              ((*pwm_value < 10) ? "0" : "") + to_string(*pwm_value),
                          20, 52, 0xFF);
            ssd->drawText("Up: Prev. pin", 28, 84, 0xFF);
            ssd->drawText("Down: Next pin", 24, 97, 0xFF);
            ssd->show();

            start_timeout = getCurrentMillis();
        }

        fiber_sleep(1);
    }

    for (int i = 0; i < 19; ++i) {
        microbit_pins[i]->setDigitalValue(0);
    }

    btnA->getDigitalValue();
    btnB->getDigitalValue();
}

void show_analog_microbit()
{
    constexpr uint16_t bar_size = 100 - 14;
    uint32_t start_timeout      = 0;

    for (uint8_t i = 0; i < 3; i++) {
        microbit_analog_pad_pins[i]->getAnalogValue();
    }

    while (1) {
        if (click_button(btnMenu)) {
            break;
        }

        if (getCurrentMillis() - start_timeout > 50) {
            uint16_t height_bar_p0 = (float(microbit_analog_pad_pins[0]->getAnalogValue()) / 1024.0) * bar_size;
            // uint16_t height_bar_p1 = (float(microbit_analog_pad_pins[1]->getAnalogValue()) / 1024.0) * bar_size;
            uint16_t height_bar_p2 = (float(microbit_analog_pad_pins[2]->getAnalogValue()) / 1024.0) * bar_size;

            printf("P0 %i/1024 => %f => %u\n", microbit_analog_pad_pins[0]->getAnalogValue(),
                   (float(microbit_analog_pad_pins[0]->getAnalogValue()) / 1024.0), height_bar_p0);
            // printf("P1 %i/1024 => %f => %u\n", microbit_analog_pad_pins[0]->getAnalogValue(),
            //        (float(microbit_analog_pad_pins[1]->getAnalogValue()) / 1024.0), height_bar_p1);
            printf("P2 %i/1024 => %f => %u\n", microbit_analog_pad_pins[0]->getAnalogValue(),
                   (float(microbit_analog_pad_pins[2]->getAnalogValue()) / 1024.0), height_bar_p2);
            printf("\n");

            ssd->fill(0x00);
            ssd->drawRectangle(30, 14, 40, 100, false, 0xFF);
            // ssd->drawRectangle(57, 14, 67, 100, false, 0xFF);
            ssd->drawRectangle(84, 14, 94, 100, false, 0xFF);

            ssd->drawRectangle(30, 100 - height_bar_p0, 40, 100, true, 0xFF);
            // ssd->drawRectangle(57, 100 - height_bar_p1, 67, 100, true, 0xFF);
            ssd->drawRectangle(84, 100 - height_bar_p2, 94, 100, true, 0xFF);

            ssd->drawText("P0", 30, 103, 0xFF);
            // ssd->drawText("P1", 57, 103, 0xFF);
            ssd->drawText("P2", 84, 103, 0xFF);
            ssd->show();

            start_timeout = getCurrentMillis();
        }

        fiber_sleep(1);
    }
}

void show_jacdac()
{
    uint32_t timeout     = 0;
    uint8_t data_to_send = 0;
    char buff[3]         = {0};
    bool is_tx_mode      = true;
    jacdac->setMode(SingleWireMode::SingleWireTx);

    timeout = getCurrentMillis();

    while (1) {
        if (click_button(btnMenu)) {
            break;
        }

        if (click_button(btnA)) {
            if (is_tx_mode) {
                is_tx_mode = false;
                jacdac->setMode(SingleWireMode::SingleWireRx);
                memset(buff, 'x', 3);
            }
            else {
                is_tx_mode = true;
                jacdac->setMode(SingleWireMode::SingleWireTx);
                data_to_send = 0;
            }

            timeout = getCurrentMillis();
        }

        if (is_tx_mode) {
            if (getCurrentMillis() - timeout > 1'000) {
                data_to_send++;
                timeout = getCurrentMillis();
            }

            sprintf(buff, "%03u", data_to_send);

            ssd->fill(0x00);
            ssd->drawText("Mode: Tx", 40, 24, 0xFF);
            ssd->drawText("Send value:", 34, 51, 0xFF);
            ssd->drawText(buff, 56, 62, 0xFF);
            ssd->drawText("A: Change to Rx", 19, 95, 0xFF);
            ssd->show();

            jacdac->putc(data_to_send);
        }
        else {
            uint8_t rcv = jacdac->getc();

            if (rcv != DEVICE_NO_DATA) {
                sprintf(buff, "%03u", rcv);
            }

            ssd->fill(0x00);
            ssd->drawText("Mode: Rx", 40, 24, 0xFF);
            ssd->drawText("Read value:", 34, 51, 0xFF);
            ssd->drawText(buff, 56, 62, 0xFF);
            ssd->drawText("A: Change to Tx", 19, 95, 0xFF);
            ssd->show();
        }

        fiber_sleep(100);
    }
}

void show_flash()
{
    uint32_t start_timeout_screen = 0;
    uint32_t start_timeout_data   = 0;
    uint16_t angle_offset         = 0;
    char buffer[32];

    ssd->fill(0x00);
    ssd->drawText("Log. distance data", 11, 40, 0xFF);
    ssd->drawText("Unplug & replug", 20, 81, 0xFF);
    ssd->drawText("the board", 35, 91, 0xFF);
    ssd->drawText("to get the file", 20, 102, 0xFF);

    ssd->drawArc(62, 63, 10, angle_offset, angle_offset + 90, 0xFF);

    ssd->show();

    flash->clearFlash();
    fiber_sleep(1'000);
    flash->setFilename("DEMO", "CSV");
    flash->writeString("time;distance\n");

    while (1) {
        if (click_button(btnMenu)) {
            break;
        }

        if (getCurrentMillis() - start_timeout_data >= 1'000) {
            sprintf(buffer, "%u;%u\n", getCurrentMillis(), tof->getDistance());
            flash->writeString(buffer);

            start_timeout_data = getCurrentMillis();
        }

        if (getCurrentMillis() - start_timeout_screen >= 50) {
            ssd->drawRectangle(51, 52, 73, 73, true, 0x00);
            ssd->drawArc(62, 63, 10, angle_offset, angle_offset + 90, 0xFF);
            ssd->show();

            angle_offset += 18;

            if (angle_offset >= 360) {
                angle_offset = 0;
            }
            start_timeout_screen = getCurrentMillis();
        }

        fiber_sleep(1);
    }
}

void show_qwic()
{
    uint32_t timeout_send = 0;

    ssd->fill(0x00);
    ssd->drawText("QWIIC", 48, 17, 0xFF);
    ssd->drawText("Send data 0x55", 21, 53, 0xFF);
    ssd->drawText("on address 0x96", 18, 64, 0xFF);
    ssd->drawText("every seconds.", 22, 75, 0xFF);
    ssd->show();

    while (1) {
        if (click_button(btnMenu)) {
            break;
        }

        if (getCurrentMillis() - timeout_send >= 1'000) {
            i2c_qwiic->beginTransmission(0xAA);
            i2c_qwiic->write(0x96);
            i2c_qwiic->endTransmission();

            timeout_send = getCurrentMillis();
        }

        fiber_sleep(1);
    }
}

void Demo_main(codal::STeaMi& steami)
{
    i2c_qwiic = &steami.i2cExt;
    spi       = &steami.spiInt;
    cs        = &steami.io.csDisplay;
    dc        = &steami.io.misoDisplay;
    rst       = &steami.io.resetDisplay;
    ssd       = new SSD1327_SPI(*spi, *cs, *dc, *rst, 128, 128);
    btnMenu   = &steami.io.buttonMenu;
    btnA      = &steami.io.buttonA;
    btnB      = &steami.io.buttonB;
    buzzer    = &steami.io.speaker;

    led_red   = &steami.io.ledRed;
    led_green = &steami.io.ledGreen;
    led_blue  = &steami.io.ledBlue;

    buzzer->setAnalogValue(0);

    steami.serial.init(115'200);
    steami.sleep(500);

    printf("Init peripherals !\r\n");

    ssd->init();
    ssd->fill(0x00);
    ssd->drawText("Initialization...", 15, 60, 0xFF);
    ssd->show();

    battery = new BQ27441(&steami.i2cInt);
    mcp     = new MCP23009E(steami.i2cInt, 0x40, steami.io.resetExpander, steami.io.irqExpander);
    hts     = new HTS221(&steami.i2cInt, 0xBE);
    pres    = new WSEN_PADS(steami.i2cInt, 0xBA);
    apds    = new APDS9960(steami.i2cInt, 0x72);
    tof     = new VL53L1X(&steami.i2cInt);
    ism     = new ISM330DL(&steami.i2cInt);
    lis     = new LIS2MDL(&steami.i2cInt);
    sai     = new STM32SAI(&steami.io.microphone, &steami.io.runmic, GPIO_AF3_SAI1, AUDIO_BUFFER);
    jacdac  = new STM32SingleWireSerial(steami.io.jacdacTx);
    rtc     = new STM32RTC();
    flash   = new DaplinkFlash(steami.i2cInt);

    battery->init();

    mcp->setup(MCP_GPIO_1, MCP_DIR::INPUT, MCP_PULLUP::PULLUP);
    mcp->setup(MCP_GPIO_2, MCP_DIR::INPUT, MCP_PULLUP::PULLUP);
    mcp->setup(MCP_GPIO_3, MCP_DIR::INPUT, MCP_PULLUP::PULLUP);
    mcp->setup(MCP_GPIO_4, MCP_DIR::INPUT, MCP_PULLUP::PULLUP);
    mcp->setup(MCP_GP_RIGHT, MCP_DIR::INPUT);
    mcp->setup(MCP_GP_BOTTOM, MCP_DIR::INPUT);
    mcp->setup(MCP_GP_LEFT, MCP_DIR::INPUT);
    mcp->setup(MCP_GP_UP, MCP_DIR::INPUT);

    hts->init();
    hts->setOutputRate(codal::HTS221_OUTPUT_RATE::RATE_7HZ);

    pres->init();

    tof->init();

    ism->init();
    ism->setAccelerometerODR(ISM_ODR::F_1_66_KHZ);
    ism->setGyroscopeODR(ISM_ODR::F_208_HZ);

    lis->init();
    lis->setODR(LIS2_ODR::F_50_HZ);
    lis->setLowPassFilter(true);

    apds->init();

    if (!sai->init()) {
        printf("Failed to init SAI\r\n");
    }
    sai->onReceiveData(micro_on_data);

    jacdac->init(115'200);
    jacdac->setMode(SingleWireMode::SingleWireTx);

    rtc->init();

    microbit_pins[0]            = &steami.io.p0;
    microbit_pins[1]            = &steami.io.p1;
    microbit_pins[2]            = &steami.io.p2;
    microbit_pins[3]            = &steami.io.p3;
    microbit_pins[4]            = &steami.io.p4;
    microbit_pins[5]            = &steami.io.p5;
    microbit_pins[6]            = &steami.io.p6;
    microbit_pins[7]            = &steami.io.p7;
    microbit_pins[8]            = &steami.io.p8;
    microbit_pins[9]            = &steami.io.p9;
    microbit_pins[10]           = &steami.io.p10;
    microbit_pins[11]           = &steami.io.p11;
    microbit_pins[12]           = &steami.io.p12;
    microbit_pins[13]           = &steami.io.p13;
    microbit_pins[14]           = &steami.io.p14;
    microbit_pins[15]           = &steami.io.p15;
    microbit_pins[16]           = &steami.io.p16;
    microbit_pins[17]           = &steami.io.p19;
    microbit_pins[18]           = &steami.io.p20;

    microbit_pwm_pins[0]        = &steami.io.p7;
    microbit_pwm_pins[1]        = &steami.io.p10;
    microbit_pwm_pins[2]        = &steami.io.p11;

    microbit_analog_pad_pins[0] = &steami.io.p0;
    microbit_analog_pad_pins[1] = &steami.io.p1;
    microbit_analog_pad_pins[2] = &steami.io.p2;

    steami.sleep(500);

    ssd->fill(0x00);
    ssd->drawText("Hello STeaMi !", 20, 60, 0xFF);
    ssd->show();
    printf("Hello STeaMi !\r\n");
    steami.sleep(1'500);

    vector<MenuEntry> mainMenuEntries = {{"Temp & Hum", []() -> void { show_temp_hum(); }},
                                         {"Pressure", []() -> void { show_pressure(); }},
                                         {"Time Of Flight", []() -> void { show_tof(); }},
                                         {"Acc / Gyro / Magn", []() -> void { show_acc_gyro_magn(); }},
                                         {"Color Sensor", []() -> void { show_optical_sensor(); }},
                                         {"Microphone", []() -> void { show_microphone(); }},
                                         {"RTC", []() -> void { show_rtc(); }},
                                         {"Buzzer", []() -> void { show_buzzer(); }},
                                         {"Buttons", []() -> void { show_button(); }},
                                         {"LEDs", []() -> void { show_rgb(); }},
                                         {"Screen", []() -> void { show_screen(); }},
                                         {"Battery", []() -> void { show_battery(); }},
                                         {"Pads \"tete\"", []() -> void { show_pads(); }},
                                         {"Pads micro:bit", []() -> void { show_microbit(); }},
                                         {"PWM micro:bit", []() -> void { show_pwm_microbit(); }},
                                         {"Analog micro:bit", []() -> void { show_analog_microbit(); }},
                                         {"jacdac (SWS)", []() -> void { show_jacdac(); }},
                                         {"DapLink Flash", []() -> void { show_flash(); }},
                                         {"QWIIC", []() -> void { show_qwic(); }}};
    mainMenu                          = new ScreenMenu(*ssd, mainMenuEntries);

    while (1) {
        show_main_menu();
    }
}