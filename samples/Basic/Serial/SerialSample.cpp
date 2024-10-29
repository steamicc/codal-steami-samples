#include "SerialSample.h"

#include <string>

#include "STM32Serial.h"

void SerialSample_main(codal::STeaMi& steami)
{
    steami.serial.init(115'200);

    printf("\r\n");
    printf("*******************************************\r\n");
    printf("* Demonstration de la communication serie *\r\n");
    printf("*******************************************\r\n");
    std::string str;
    bool state = false;

    while (true) {
        steami.io.ledGreen.setDigitalValue(state ? 1 : 0);

        if (steami.serial.isReadable() != 0) {
            char c = static_cast<char>(steami.serial.getChar(codal::ASYNC));
            str += c;
            printf("< %c \r\n", c);
            printf("> %s \r\n", str.c_str());
        }

        steami.sleep(250);
        state = !state;
    }
}
