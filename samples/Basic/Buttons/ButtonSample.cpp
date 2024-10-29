#include "ButtonSample.h"

using namespace std;
using namespace codal;

void ButtonSample_main(codal::STeaMi& steami)
{
    STM32Pin& led_red   = steami.io.ledRed;
    STM32Pin& led_green = steami.io.ledGreen;
    STM32Pin& led_blue  = steami.io.ledBlue;

    STM32Pin& btn_A     = steami.io.buttonA;
    STM32Pin& btn_B     = steami.io.buttonB;
    STM32Pin& btn_M     = steami.io.buttonMenu;

    while (true) {
        led_red.setDigitalValue(btn_A.getDigitalValue() ? 0 : 1);
        led_green.setDigitalValue(btn_B.getDigitalValue() ? 0 : 1);
        led_blue.setDigitalValue(btn_M.getDigitalValue() ? 0 : 1);
        steami.sleep(50);
    }
}
