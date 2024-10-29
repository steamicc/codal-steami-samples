#pragma once

#include <STM32Pin.h>
#include <string.h>

// constexpr uint8_t MCP_GPIO_1    = 0;
// constexpr uint8_t MCP_GPIO_2    = 1;
// constexpr uint8_t MCP_GPIO_3    = 2;
// constexpr uint8_t MCP_GPIO_4    = 3;
constexpr uint8_t MCP_GP_RIGHT  = 4;
constexpr uint8_t MCP_GP_BOTTOM = 5;
constexpr uint8_t MCP_GP_LEFT   = 6;
constexpr uint8_t MCP_GP_UP     = 7;

std::string fToStr(float value, unsigned pres)
{
    int ent = (int)value;
    int dec = (int)((value - ent) * pow(10, pres));

    return std::to_string(ent) + "." + to_string(abs(dec));
}

bool click_button(codal::STM32Pin* btn)
{
    if (btn->getDigitalValue() == 0) {
        while (btn->getDigitalValue() == 0);

        return true;
    }

    return false;
}

int wrap(int value, int min, int max)
{
    if (value < min)
        return max - (min - value);
    else if (value > max)
        return min + (value - max);
    else
        return value;
}

float clampf(float value, float min, float max)
{
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}