
#ifndef __HTS221_SAMPLE_H__
#define __HTS221_SAMPLE_H__

#include "STeaMi.h"
#include "VL53L1X.h"

#ifndef SAMPLE_MAIN
    #define SAMPLE_MAIN vlx53l1xSample
#endif

void vlx53l1xSample(codal::STeaMi& steami);

#endif
