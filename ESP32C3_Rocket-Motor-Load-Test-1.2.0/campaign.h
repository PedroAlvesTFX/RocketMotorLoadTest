#ifndef CAMPAIGN_H
#define CAMPAIGN_H

#include <Arduino.h>
#include "types.h"

// 30 s de ensaio + 2 s de pré-trigger a 80 SPS, com pequena margem.
#define MAX_SAMPLES 2700

class Campaign
{
public:
    void begin();
    void clear();
    void start();
    void stop();

    bool addSample(uint32_t us, float grams);

    uint32_t getCount() const;
    const Sample *data() const;
    float getPeak() const;
    bool isRecording() const;

private:
    Sample samples[MAX_SAMPLES];
    uint32_t sampleCount;
    float peak;
    bool recording;
};

extern Campaign campaign;

#endif
