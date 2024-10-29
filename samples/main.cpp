#include "STeaMi.h"

#if defined(BUTTONS_SAMPLE)
    #include "ButtonSample.h"
#elif defined(SERIAL_SAMPLE)
    #include "SerialSample.h"
#elif defined(SIGLE_WIRE_SERIAL_SAMPLE)
    #include "SingleWireSerialSample.h"
#elif defined(SAI_SAMPLE)
    #include "SAI_sample.h"
#elif defined(VL53L1X_SAMPLE)
    #include "VL53L1X_sample.h"
#elif defined(HTS221_SAMPLE)
    #include "HTS221_sample.h"
#elif defined(WSEN_PADS_SAMPLE)
    #include "WSEN-PADS_sample.h"
#elif defined(SCANNER_I2C)
    #include "ScannerI2C.h"
#elif defined(OLED_SSD1327)
    #include "OLED_SSD1327.h"
#elif defined(APDS9960)
    #include "APDS9960_sample.h"
#elif defined(BQ27441)
    #include "BQ27441_sample.h"
#elif defined(RTC_SAMPLE)
    #include "RTC_sample.h"
#elif defined(DAPLINK_FLASH)
    #include "DapLink_Flash_sample.h"
#elif defined(DEMO)
    #include "demo_sample.h"
#elif defined(FUS_WS_OPERATOR)
    #include "FUS_WS_Operator.h"
#elif defined(BLE_BROADCAST)
    #include "BLE_Broadcast_Sample.h"
#elif defined(OOB)
    #include "oob.h"
#else
    #include "BlinkSample.h"
#endif

codal::STeaMi steami;

auto main() -> int
{
    steami.init();
    SAMPLE_MAIN(steami);
    codal::release_fiber();
    return 0;
}