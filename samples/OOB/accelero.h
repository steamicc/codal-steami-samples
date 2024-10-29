#pragma once

#include "ism330dl.h"
#include "utils.h"

extern codal::STM32Pin* btnMenu;
extern codal::ISM330DL* ism;
extern codal::SSD1327_SPI* ssd;

class Ball {
  public:
    int x;
    int y;
    float ax;
    float ay;
    float speed;
    int radius;

    Ball(int x, int y, int radius) : x{x}, y{y}, ax{0}, ay{0}, speed{3}, radius{radius} {}

    float low_limit() { return 18 + radius; }
    float high_limit() { return 110 - radius; }

    void update(ISM_Data& accel)
    {
        ax = clampf(ax + (accel.y), -5, 5);
        ay = clampf(ay - (accel.x), -5, 5);

        x += ax * speed;
        y += ay * speed;

        if (x < low_limit() || x > high_limit()) {
            ax *= -0.8;
            x = clampf(x, low_limit(), high_limit());
        }

        if (y < low_limit() || y > high_limit()) {
            ay *= -0.8;
            y = clampf(y, low_limit(), high_limit());
        }
    }

    void draw() { ssd->drawCircle(x, y, radius, false, 0xFF); }
};

void accelero_prog()
{
    Ball ball(64, 64, 8);
    while (true) {
        if (click_button(btnMenu)) {
            break;
        }

        auto accel = ism->readAccelerometerData();

        ball.update(accel);

        ssd->fill(0x00);
        ssd->drawRectangle(18, 18, 110, 110, false, 0xFF);
        ball.draw();
        ssd->show();
    }
}