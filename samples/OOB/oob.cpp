#include "oob.h"

using namespace codal;
using namespace std;

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include "APDS9960.h"
#include "HTS221.h"
#include "STM32Pin.h"
#include "STM32RTC.h"
#include "STM32SAI.h"
#include "VL53L1X.h"
#include "WSEN-PADS.h"
#include "ism330dl.h"
#include "lis2mdl.h"
#include "mcp23009-e.h"
#include "menu.h"
#include "pcm_utils.h"
#include "pdm2pcm.h"
#include "ssd1327.h"
// #include "STM32SingleWireSerial.h"

#include "accelero.h"
#include "buzzer.h"
#include "compass.h"
#include "game.h"
#include "rgb.h"
#include "sensors.h"
#include "utils.h"

typedef void (*prog_func)(void);

STM32SPI* spi       = nullptr;
STM32Pin* cs        = nullptr;
STM32Pin* dc        = nullptr;
STM32Pin* rst       = nullptr;
STM32Pin* led_red   = nullptr;
STM32Pin* led_green = nullptr;
STM32Pin* led_blue  = nullptr;
SSD1327_SPI* ssd    = nullptr;

STM32Pin* btnMenu   = nullptr;
STM32Pin* btnA      = nullptr;
STM32Pin* btnB      = nullptr;
STM32Pin* buzzer    = nullptr;

MCP23009E* mcp      = nullptr;
HTS221* hts         = nullptr;
WSEN_PADS* pres     = nullptr;
VL53L1X* tof        = nullptr;
ISM330DL* ism       = nullptr;
LIS2MDL* lis        = nullptr;

void Demo_OOB(codal::STeaMi& steami)
{
    spi       = &steami.spiExt;
    cs        = &steami.io.csDisplay;
    dc        = &steami.io.misoDisplay;
    rst       = &steami.io.resetDisplay;
    ssd       = new SSD1327_SPI(*spi, *cs, *dc, *rst, 128, 128);
    btnMenu   = &steami.io.buttonMenu;
    btnA      = &steami.io.buttonA;
    btnB      = &steami.io.buttonB;
    buzzer    = &steami.io.speaker;

    led_red   = &steami.io.ledRed;
    led_green = &steami.io.ledGreen;
    led_blue  = &steami.io.ledBlue;

    buzzer->setAnalogValue(0);

    steami.serial.init(115'200);
    steami.sleep(500);

    printf("Init...\r\n");

    ssd->init();
    ssd->fill(0x00);
    ssd->drawText("Initialization...", 20, 60, 0xFF);
    ssd->show();

    printf("Init. MCP23009E...\n");
    mcp = new MCP23009E(steami.i2cInt, 0x40, steami.io.resetExpander, steami.io.irqExpander);
    mcp->setup(MCP_GP_RIGHT, MCP_DIR::INPUT);
    mcp->setup(MCP_GP_BOTTOM, MCP_DIR::INPUT);
    mcp->setup(MCP_GP_LEFT, MCP_DIR::INPUT);
    mcp->setup(MCP_GP_UP, MCP_DIR::INPUT);

    printf("Init. HTS221...\n");
    hts = new HTS221(&steami.i2cInt, 0xBE);
    hts->init();
    hts->setOutputRate(codal::HTS221_OUTPUT_RATE::RATE_7HZ);

    printf("Init. WSEN_PADS...\n");
    pres = new WSEN_PADS(steami.i2cInt, 0xBA);
    pres->init();

    printf("Init. VL53L1X...\n");
    tof = new VL53L1X(&steami.i2cInt);
    tof->init();

    printf("Init. ISM330DL...\n");
    ism = new ISM330DL(&steami.i2cInt);
    ism->init();
    ism->setAccelerometerODR(ISM_ODR::F_1_66_KHZ);
    ism->setGyroscopeODR(ISM_ODR::F_208_HZ);

    printf("Init. LIS2MDL...\n");
    lis = new LIS2MDL(&steami.i2cInt);
    lis->init();

    unsigned select_prog = 0;
    vector<prog_func> progs({game, buzzer_prog, sensors_prog, rgb_prog, compass_prog, accelero_prog});

    printf("Init done\nHello STeamy !\r\n");
    ssd->fill(0x00);
    ssd->drawText("Use the 'Menu' key", 11, 46, 0xFF);
    ssd->drawText("to change demo", 22, 60, 0xFF);
    ssd->show();

    while (true) {
        if (click_button(btnMenu)) {
            break;
        }
    }
    ssd->fill(0x00);
    ssd->show();

    while (1) {
        progs[select_prog]();

        if (++select_prog >= progs.size()) {
            select_prog = 0;
        }
    }
}