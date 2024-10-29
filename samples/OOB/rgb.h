#pragma once

#include <cstdint>

extern codal::MCP23009E* mcp;
extern codal::STM32Pin* btnMenu;
extern codal::STM32Pin* led_red;
extern codal::STM32Pin* led_green;
extern codal::STM32Pin* led_blue;

void rgb_prog()
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
