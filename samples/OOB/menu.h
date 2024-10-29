#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ssd1327.h"

constexpr int MENU_POS_X                = 25;
constexpr int MENU_POS_X_CURSOR         = 15;
constexpr int MENU_MIDDLE_POS_Y         = 60;
constexpr int MENU_LINE_HEIGHT          = 9;
constexpr char MENU_SELECT_SYM          = '>';
constexpr uint8_t MENU_ENTRY_PER_SCREEN = 9;

struct MenuEntry {
    std::string name;
    std::function<void(void)> func;
};

class ScreenMenu {
  public:
    ScreenMenu(codal::SSD1327& ssd, std::vector<MenuEntry> entries)
        : ssd(ssd), entries(entries), menuOffset(0), cursorPos(0)
    {
        midMenu = std::min(4, (int)entries.size() / 2);
    }

    void moveUp()
    {
        printCursor(0x00);

        cursorPos--;

        if (cursorPos < menuOffset) {
            if (!moveDownMenu()) cursorPos = entries.size() - 1;
        }

        printCursor(0xFF);
        ssd.show();
    }

    void moveDown()
    {
        printCursor(0x00);

        cursorPos++;

        if (cursorPos >= (menuOffset + MENU_ENTRY_PER_SCREEN)) {
            if (!moveUpMenu()) cursorPos = 0;
        }

        printCursor(0xFF);
        ssd.show();
    }

    void execute() { entries[cursorPos].func(); }

    void show()
    {
        printMenu();
        printCursor(0xFF);
        ssd.show();
    }

  private:
    codal::SSD1327& ssd;
    std::vector<MenuEntry> entries;

    int8_t menuOffset;
    int8_t cursorPos;
    uint8_t midMenu;

    void printCursor(uint16_t color)
    {
        unsigned midPoint = MENU_MIDDLE_POS_Y - midMenu * MENU_LINE_HEIGHT;

        ssd.drawChar(MENU_SELECT_SYM, MENU_POS_X_CURSOR,
                     midPoint + (cursorPos * MENU_LINE_HEIGHT) - (menuOffset * MENU_LINE_HEIGHT), color);
    }

    void printMenu()
    {
        ssd.fill(0x00);

        unsigned midPoint = MENU_MIDDLE_POS_Y - midMenu * MENU_LINE_HEIGHT;

        for (uint8_t i = menuOffset; i < menuOffset + MENU_ENTRY_PER_SCREEN; ++i) {
            unsigned relativePos = (i * MENU_LINE_HEIGHT) - (menuOffset * MENU_LINE_HEIGHT);
            ssd.drawText(entries[i].name, MENU_POS_X, midPoint + relativePos, 0xFF);
        }
    }

    bool moveUpMenu()
    {
        bool result = true;
        menuOffset++;

        if ((menuOffset + MENU_ENTRY_PER_SCREEN) > (int8_t)entries.size()) {
            menuOffset = 0;
            result     = false;
        }

        printMenu();
        return result;
    }

    bool moveDownMenu()
    {
        bool result = true;
        menuOffset--;

        if (menuOffset < 0) {
            menuOffset = std::max(0, (int8_t)entries.size() - MENU_ENTRY_PER_SCREEN);
            result     = false;
        }

        printMenu();
        return result;
    }
};