
#include "HTS221_sample.h"

#include <string>

void hts221Sample(codal::STeaMi& steami)
{
    steami.serial.init(115'200);

    printf("\r\n");
    printf("*******************************************\r\n");
    printf("*          Demonstration du HTS221        *\r\n");
    printf("*******************************************\r\n");

    codal::HTS221 hts221(&steami.i2cInt, 0xBE);
    hts221.init();

    hts221.setOutputRate(codal::HTS221_OUTPUT_RATE::RATE_7HZ);

    steami.sleep(2'000);

    std::string temperature;
    std::string humidity;

    bool state = false;

    while (true) {
        state = !state;
        steami.io.ledGreen.setDigitalValue(state ? 1 : 0);

        if (!hts221.isTemperatureDataAvailable() || !hts221.isHumidityDataAvailable()) {
            steami.sleep(100);
            printf("Sensor is not ready\r\n");
            continue;
        }

        temperature = "Temperature : " + std::to_string(hts221.getTemperature()) + " C";
        humidity    = "Humidity : " + std::to_string(hts221.getHumidity()) + " %RH";

        printf("%s \r\n", temperature.c_str());
        printf("%s \r\n", humidity.c_str());
        printf("\r\n");

        steami.sleep(1'000);
    }
}
