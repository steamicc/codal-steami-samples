#pragma once

#include <cmath>
#include <vector>

#include "clock.h"
#include "ssd1327.h"
#include "utils.h"

constexpr double DEG_TO_RAD = 0.017452792;

extern codal::STM32Pin* btnMenu;
extern codal::STM32Pin* btnA;
extern codal::STM32Pin* btnB;
extern codal::SSD1327_SPI* ssd;
extern codal::MCP23009E* mcp;

class Earth {
  public:
    int size;
    bool is_explode;
    Earth(int size) : size{size}, is_explode{false} {}
    void draw()
    {
        ssd->drawCircle(64, 64, size, true, 0xFF);
        ssd->drawText("Home", 53, 60, 0x00);
    }
};

class Ship {
  public:
    int angle;
    unsigned cx;
    unsigned cy;
    unsigned size;
    unsigned speed;
    Earth& earth;

    Ship(Earth& earth, unsigned speed) : angle{0}, cx{0}, cy{0}, size{5}, speed{speed}, earth{earth}
    {
        compute_center();
    }

    void move_left()
    {
        angle = wrap(angle - speed, 0, 359);
        compute_center();
    }
    void move_right()
    {
        angle = wrap(angle + speed, 0, 359);
        compute_center();
    }

    void compute_center()
    {
        double c = cos(double(angle) * DEG_TO_RAD);
        double s = sin(double(angle) * DEG_TO_RAD);
        cx       = 64 + (earth.size + 5) * c;
        cy       = 64 + (earth.size + 5) * s;
    }

    void draw()
    {
        double Acos = cos(double(angle) * DEG_TO_RAD);
        double Asin = sin(double(angle) * DEG_TO_RAD);
        double Bcos = cos(double(angle - 120) * DEG_TO_RAD);
        double Bsin = sin(double(angle - 120) * DEG_TO_RAD);
        double Ccos = cos(double(angle + 120) * DEG_TO_RAD);
        double Csin = sin(double(angle + 120) * DEG_TO_RAD);

        ssd->drawSegment(cx + (size * Bcos), cy + (size * Bsin), cx + (size * Acos), cy + (size * Asin), 1, 0xFF);
        ssd->drawSegment(cx + (size * Ccos), cy + (size * Csin), cx + (size * Acos), cy + (size * Asin), 1, 0xFF);
        ssd->drawSegment(cx + (size * Ccos), cy + (size * Csin), cx + (size * Bcos), cy + (size * Bsin), 1, 0xFF);
    }
};

class Enemy {
  public:
    float cosine;
    float sine;
    unsigned size;
    float distance;
    bool is_dead;

    Enemy(unsigned size) : size{size}, distance{64}, is_dead{false}
    {
        int angle = target_random(360);
        cosine    = cos(angle * DEG_TO_RAD);
        sine      = sin(angle * DEG_TO_RAD);
    }

    void draw()
    {
        if (is_dead) {
            return;
        }

        ssd->drawPolygon(pos_x(), pos_y(), 5, size, 1, 0xFF);
    }

    int pos_x() { return 64 + distance * cosine; }

    int pos_y() { return 64 + distance * sine; }

    void update(Earth& earth)
    {
        if (is_dead) {
            return;
        }

        if (distance <= earth.size) {
            earth.is_explode = true;
        }

        distance -= 0.8;
    }
};

class Bullet {
  public:
    float cosine;
    float sine;
    unsigned size;
    float distance;
    bool is_out;

    Bullet(int angle, unsigned size, float distance) : size{size}, distance{distance}, is_out{false}
    {
        cosine = cos(angle * DEG_TO_RAD);
        sine   = sin(angle * DEG_TO_RAD);
    }

    void draw()
    {
        if (is_out) {
            return;
        }
        ssd->drawCircle(pos_x(), pos_y(), size, true, 0xFF);
    }

    int pos_x() { return 64 + distance * cosine; }

    int pos_y() { return 64 + distance * sine; }

    void update(Enemy& enemy)
    {
        if (is_out) {
            return;
        }

        int x      = pow(pos_x() - enemy.pos_x(), 2);
        int y      = pow(pos_y() - enemy.pos_y(), 2);
        int radius = pow(max(size, enemy.size), 2);

        if (x + y <= radius) {
            enemy.is_dead = true;
            is_out        = true;
        }
        else {
            distance += 2;
            is_out = distance > (64 + size);
        }
    }
};

void game()
{
    Earth earth(15);
    Ship ship(earth, 8);
    Enemy enemy(5);
    std::vector<Bullet> bullets(3, Bullet(0, 0, 200));

    uint32_t score  = 0;
    bool is_playing = true;
    bool can_shoot  = true;
    target_seed_random(getCurrentMillis());

    while (true) {
        if (click_button(btnMenu)) {
            break;
        }

        if (is_playing) {
            if (mcp->getLevel(MCP_GP_RIGHT) == codal::MCP_LOGIC_LEVEL::LOW) {
                ship.move_right();
            }
            else if (mcp->getLevel(MCP_GP_LEFT) == codal::MCP_LOGIC_LEVEL::LOW) {
                ship.move_left();
            }

            if (btnA->getDigitalValue() == 0 && can_shoot) {
                for (unsigned i = 0; i < bullets.size(); ++i) {
                    if (bullets[i].is_out) {
                        bullets[i] = Bullet(ship.angle, 1, earth.size + 5);
                        can_shoot  = false;
                        break;
                    }
                }
            }
            else if (btnA->getDigitalValue() == 1) {
                can_shoot = true;
            }

            if (enemy.is_dead) {
                score++;
                enemy = Enemy(enemy.size);
            }

            if (earth.is_explode) {
                is_playing = false;
                continue;
            }

            for (unsigned i = 0; i < bullets.size(); ++i) {
                bullets[i].update(enemy);
            }

            enemy.update(earth);

            ssd->fill(0x00);

            if (score >= 100)
                ssd->drawText(std::to_string(score), 56, 2, 0xFF);
            else if (score >= 10)
                ssd->drawText(std::to_string(score), 59, 2, 0xFF);
            else
                ssd->drawText(std::to_string(score), 62, 2, 0xFF);

            earth.draw();
            ship.draw();

            for (unsigned i = 0; i < bullets.size(); ++i) {
                bullets[i].draw();
            }
            enemy.draw();

            ssd->show();
        }
        else {
            if (click_button(btnB)) {
                score            = 0;
                is_playing       = true;
                earth.is_explode = false;
                enemy.is_dead    = true;
                for (unsigned i = 0; i < bullets.size(); ++i) {
                    bullets[i].is_out = true;
                }
            }

            ssd->fill(0x00);
            ssd->drawRectangle(38, 28, 95, 39, true, 0xFF);
            ssd->drawText("GAME OVER", 40, 30, 0x00);
            ssd->drawText("Score: " + std::to_string(score), 35, 50, 0xFF);
            ssd->drawText("Press \"B\"", 27, 78, 0xFF);
            ssd->drawText("ro restart !", 31, 89, 0xFF);
            ssd->show();
        }
    }

    ssd->fill(0x00);
    ssd->show();
}