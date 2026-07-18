#include "campaign.h"

Campaign campaign;

void Campaign::begin()
{
    clear();
}

void Campaign::clear()
{
    sampleCount = 0;
    peak = 0.0f;
    recording = false;
}

void Campaign::start()
{
    clear();
    recording = true;
}

void Campaign::stop()
{
    recording = false;
}

bool Campaign::addSample(uint32_t us, float grams)
{
    if (!recording || sampleCount >= MAX_SAMPLES)
        return false;

    samples[sampleCount].us = us;
    samples[sampleCount].grams = grams;

    if (sampleCount == 0 || grams > peak)
        peak = grams;

    sampleCount++;
    return true;
}

uint32_t Campaign::getCount() const
{
    return sampleCount;
}

const Sample *Campaign::data() const
{
    return samples;
}

float Campaign::getPeak() const
{
    return peak;
}

bool Campaign::isRecording() const
{
    return recording;
}
