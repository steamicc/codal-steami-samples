#include "BLE_Broadcast_Sample.h"

#include <cstdio>

#include "AdvertisingData.h"
#include "AdvertisingFlagsBuilder.h"
#include "BLEDevice.h"
#include "HCI_SharedMemory.h"
#include "STM32Serial.h"
#include "ble_utils.h"

void BLE_Broadcast_Sample_main(codal::STeaMi& steami)
{
    steami.serial.init(115'200);

    printf("\r\n");
    printf("*******************************************\r\n");
    printf("*      Demonstration broadcast du BLE     *\r\n");
    printf("*******************************************\r\n");

    HCI_SharedMemory hci;
    BLEDevice ble(&hci);

    AdvertisingData adv;
    AdvertisingData advScan;

    // hci.enableDebug();
    ble.init();

    uint8_t flags = AdvertisingFlagsBuilder().addBrEdrNotSupported().addLeGeneralDiscoverableMode().build();
    adv.setFlags(flags);
    adv.setLocalName("Broadcast test");
    adv.setUserData("Coucou !");

    advScan.setUserData("Hi ! I'm scan response !");

    ble.setAdvertisingData(adv);
    ble.setScanResponseData(advScan);

    if (ble.startAdvertising() != BLEDeviceError::SUCCESS) {
        printf("Failed to start BLE !");
    }

    while (1) {
        steami.sleep(1'000);

        adv.setUserData(std::to_string(getCurrentMillis() / 1'000) + " sec");
        ble.setAdvertisingData(adv);
    }
}
