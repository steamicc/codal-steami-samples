#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

#include "lis2mdl.h"
#include "utils.h"

extern codal::STM32Pin* btnMenu;
extern codal::STM32Pin* btnA;
extern codal::LIS2MDL* lis;
extern codal::SSD1327_SPI* ssd;

constexpr float TRI_ANGLE = 155.0 * 0.017452792;

float mapf(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

Lis2Data average_measure(unsigned nb_measure)
{
    Lis2Data data{.x = 0, .y = 0, .z = 0};

    for (unsigned i = 0; i < nb_measure; ++i) {
        Lis2Data magn = lis->readData();

        data.x += magn.x;
        data.y += magn.y;
    }

    data.x /= float(nb_measure);
    data.y /= float(nb_measure);

    return data;
}

void compass_prog()
{
    float size        = 32;
    float cx          = 64;
    float cy          = 64;
    float calib_min_x = std::numeric_limits<float>::max();
    float calib_max_x = std::numeric_limits<float>::min();
    float calib_min_y = std::numeric_limits<float>::max();
    float calib_max_y = std::numeric_limits<float>::min();

    // calibration
    ssd->fill(0x00);
    ssd->drawRectangle(25, 14, 102, 40, true, 0xFF);
    ssd->drawText("CALIBRATION", 31, 18, 0x00);
    ssd->drawText("MAGNETOMETER", 28, 29, 0x00);
    ssd->drawText("Move the STeaMi", 21, 49, 0xFF);
    ssd->drawText("by making \"8\" shapes", 5, 60, 0xFF);
    ssd->drawText("Push \"A\" to finish", 12, 86, 0xFF);
    ssd->show();

    while (true) {
        if (click_button(btnA)) {
            break;
        }

        Lis2Data magn = average_measure(20);

        calib_max_x   = std::max(calib_max_x, magn.x);
        calib_min_x   = std::min(calib_min_x, magn.x);

        calib_max_y   = std::max(calib_max_y, magn.y);
        calib_min_y   = std::min(calib_min_y, magn.y);
    }

    printf("Calibration:\n\tx: [%.2f, %.2f]\n\ty: [%.2f, %.2f]\n", calib_min_x, calib_max_x, calib_min_y, calib_max_y);

    while (true) {
        if (click_button(btnMenu)) {
            break;
        }

        Lis2Data magn = average_measure(10);

        float x       = mapf(magn.x, calib_min_x, calib_max_x, -1, 1);
        float y       = -mapf(magn.y, calib_min_y, calib_max_y, -1, 1);
        float angle   = std::atan2(y, x);

        double Acos   = cos(angle);
        double Asin   = sin(angle);
        double Bcos   = cos(angle - TRI_ANGLE);
        double Bsin   = sin(angle - TRI_ANGLE);
        double Ccos   = cos(angle + TRI_ANGLE);
        double Csin   = sin(angle + TRI_ANGLE);

        ssd->fill(0x00);
        ssd->drawCircle(64, 64, 50, false, 0xFF);
        ssd->drawCircle(64, 64, 2, true, 0xFF);

        ssd->drawSegment(cx + (size * Bcos), cy + (size * Bsin), cx + (size * Acos), cy + (size * Asin), 1, 0xFF);
        ssd->drawSegment(cx + (size * Ccos), cy + (size * Csin), cx + (size * Acos), cy + (size * Asin), 1, 0xFF);
        ssd->drawSegment(cx + (size * Ccos), cy + (size * Csin), cx + (size * Bcos), cy + (size * Bsin), 1, 0xFF);

        ssd->drawText("N", 61, 2, 0xFF);
        ssd->drawText("S", 61, 119, 0xFF);
        ssd->drawText("O", 2, 61, 0xFF);
        ssd->drawText("E", 121, 61, 0xFF);
        ssd->show();

        target_wait(100);
    }
}