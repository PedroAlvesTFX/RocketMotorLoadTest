#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

enum TestState
{
    STATE_IDLE,
    STATE_ARMED,
    STATE_WAIT_TRIGGER,
    STATE_RECORDING,
    STATE_SAVING,
    STATE_FINISHED
};

struct Sample
{
    uint32_t us;
    float grams;
};

struct CampaignHeader
{
    uint32_t id;

    char datetime[32];

    uint32_t samples;

    float peak;

    float average;

    float impulse;

    float duration;
};

#endif