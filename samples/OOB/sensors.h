#pragma once

#include "HTS221.h"
#include "ssd1327.h"
#include "utils.h"

extern codal::SSD1327_SPI* ssd;
extern codal::HTS221* hts;
extern codal::WSEN_PADS* pres;
extern codal::VL53L1X* tof;

void sensors_prog()
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
        ssd->drawText("Temperature: " + fToStr(hts->getTemperature(), 2) + " C", 5, 50, 0xFF);
        ssd->drawText("Humidity: " + fToStr(hts->getHumidity(), 2) + " %HR", 5, 60, 0xFF);
        ssd->drawText("Pressure: " + fToStr(pres->getPressure(), 2) + " kPa", 5, 70, 0xFF);
        // ssd->drawText("Distance: " + fToStr(tof->getDistance(), 2) + " mm", 5, 80, 0xFF);

        ssd->show();
    }
}