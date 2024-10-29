
#include "ScannerI2C.h"

#include <cstdio>

void ScannerI2C(codal::STeaMi& steami)
{
    steami.serial.init(115'200);

    steami.sleep(1'000);

    printf("\r\n");
    printf("*******************************************\r\n");
    printf("*               Scanner I2C               *\r\n");
    printf("*******************************************\r\n");

    codal::STM32I2C& i2c = steami.i2cInt;

    steami.sleep(2'000);

    printf("0x  00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F\r\n");
    for (uint16_t m = 0; m < 0x100; m += 0x10) {
        printf("%2X ", m);
        for (uint8_t l = 0; l < 0x10; l++) {
            uint8_t addr = m | l;

            if ((addr & 0x01) == 0 && i2c.isDeviceAvailable(addr)) {
                printf(" %2X ", addr);
            }
            else {
                printf(" -- ");
            }
        }
        printf("\r\n");
    }

    while (true);
}
