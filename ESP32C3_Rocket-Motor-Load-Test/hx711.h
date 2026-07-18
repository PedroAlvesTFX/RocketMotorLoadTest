#ifndef HX711_DRIVER_H
#define HX711_DRIVER_H

#include <Arduino.h>
#include <HX711_ADC.h>

#include "config.h"


class HX711Driver
{

public:

    HX711Driver();

    void begin();

    void tare();

    bool update();

    float getGrams();

    long getRaw();

    bool isInverted() const;

    bool setInverted(bool inverted);


private:

    HX711_ADC loadCell;

    float grams;

    long raw;

    bool inverted;

};


extern HX711Driver Scale;


#endif