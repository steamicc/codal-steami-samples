#pragma once

#include <cstdint>

#include "utils.h"

extern codal::STM32Pin* btnMenu;
extern codal::STM32Pin* buzzer;
extern codal::SSD1327_SPI* ssd;
extern codal::MCP23009E* mcp;

void buzzer_prog()
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