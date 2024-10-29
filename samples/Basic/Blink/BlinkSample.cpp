#include "BlinkSample.h"

using namespace std;
using namespace codal;

void BlinkSample_main(codal::STeaMi& steami)
{
    bool state = false;

    while (true) {
        steami.io.ledBlue.setDigitalValue((int)state);
        steami.io.ledRed.setDigitalValue((int)!state);

        steami.sleep(500);
        state = !state;
    }
}
