#include "WSEN-PADS_sample.h"

#include <string>

void Wsen_PadsSample(codal::STeaMi& steami)
{
    steami.serial.init(115'200);

    printf("\r\n");
    printf("*******************************************\r\n");
    printf("*        Demonstration du WSEN_PADS       *\r\n");
    printf("*******************************************\r\n");

    codal::WSEN_PADS press(steami.i2cInt, 0xBA);
    press.init();

    steami.sleep(2'000);

    printf("Device ID : %X\r\n", press.whoAmI());

    steami.sleep(2'000);

    std::string pressure;
    std::string temperature;

    bool state = false;

    while (true) {
        state = !state;
        steami.io.ledGreen.setDigitalValue(state ? 1 : 0);

        pressure    = "Pressure : " + std::to_string(press.getPressure()) + " kPa";
        temperature = "Temperature : " + std::to_string(press.getTemperature()) + " C";

        printf("%s \r\n", pressure.c_str());
        printf("%s \r\n", temperature.c_str());
        printf("\r\n");

        steami.sleep(1'000);
    }
}
