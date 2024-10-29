
#include "BQ27441_sample.h"

#include <string>

#include "BQ27441.h"

void BQ27441_SampleMain(codal::STeaMi& steami)
{
    steami.serial.init(115'200);
    steami.sleep(2'000);

    printf("\r\n");
    printf("*******************************************\r\n");
    printf("*        Demonstration du BQ27441         *\r\n");
    printf("*******************************************\r\n");

    uint32_t start_time = getCurrentMillis();
    codal::BQ27441 gauge(&steami.i2cInt);
    gauge.init();

    while (!gauge.is_init()) {
        printf("Wait for gauge init...\n");
        steami.sleep(500);

        if ((getCurrentMillis() - start_time) > 5'000) {
            printf("TIMEOUT ! The Gauge is not ready, or there is no battery\n");
            while (1);
        }
    }

    while (1) {
        printf("Device ID : 0x%04X\n", gauge.device_type());

        printf("Flags: 0x%04X\n", gauge.read_flags());
        printf("Is bat inserted : %s\n", gauge.is_battery_detected() ? "TRUE" : "FALSE");
        printf("State of charge: %d %%\n", gauge.state_of_charge());

        printf("\nTemperature: %.2f C\n", gauge.get_temperature());
        printf("Voltage: %.2f V\n", gauge.get_voltage());
        printf("Average current: %.2f A\n", gauge.get_average_current());
        printf("Average power: %.2f W\n", gauge.get_average_power());
        printf("\n\n");

        steami.sleep(5'000);
    }
}