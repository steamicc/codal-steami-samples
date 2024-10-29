#include "SAI_sample.h"

#include <cmath>
#include <cstdio>

#include "STM32SAI.h"
#include "pcm_utils.h"
#include "pdm2pcm.h"

using namespace std;
using namespace codal;

constexpr uint16_t AUDIO_BUFFER = 128;

STM32SAI* sai                   = nullptr;
STM32Pin* btnA                  = nullptr;
PDM2PCM pdm2pcm(16, 2, 0, 1);

uint16_t rawMicData[AUDIO_BUFFER];
bool processedData = true;

bool is_btn_pressed(STM32Pin* btn)
{
    if (btn->getDigitalValue() == 0) {
        while (btn->getDigitalValue() == 0);
        return true;
    }

    return false;
}

void onData(const int32_t* data)
{
    if (processedData) {
        for (uint16_t i = 0; i < AUDIO_BUFFER; ++i) {
            rawMicData[i] = uint16_t(data[i] & 0x0000FFFF);
        }

        processedData = false;
    }
}

void onError(const uint32_t errCode)
{
    printf("An error occured: %08X (%d)\r\n", errCode, errCode);
}

void SAI_main(STeaMi& steami)
{
    bool is_paused = false;

    steami.serial.init(115'200);
    steami.sleep(500);

    printf("Init\r\n");
    sai  = new STM32SAI(&steami.io.microphone, &steami.io.runmic, GPIO_AF3_SAI1, AUDIO_BUFFER);
    btnA = &steami.io.buttonA;

    if (!sai->init()) {
        printf("Failed to init SAI\r\n");
        while (1);
    }

    sai->onReceiveData(onData);
    sai->onError(onError);

    sai->startListening();

    printf("Ready !\r\n");
    while (1) {
        if (is_btn_pressed(btnA)) {
            if (is_paused) {
                printf("RESUME\r\n");
                sai->startListening();
                is_paused = false;
            }
            else {
                printf("PAUSE\r\n");
                sai->stopListening();
                is_paused = true;
            }
        }

        if (!processedData) {
            auto conv = pdm2pcm.convert(rawMicData, AUDIO_BUFFER);
            printf("Signal:%f\r\n", PCMUtils::toDecibel(conv));
            processedData = true;
        }

        fiber_sleep(1);
    }
}
