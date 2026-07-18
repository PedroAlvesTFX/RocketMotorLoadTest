#ifndef RTS_ACQUISITION_H
#define RTS_ACQUISITION_H

#include <Arduino.h>

void acquisitionBegin();

void acquisitionLoop();

float acquisitionGetForce();

float acquisitionGetPeak();

bool acquisitionIsRecording();

bool acquisitionStart();

void acquisitionStop();

void acquisitionTare();

float acquisitionGetTriggerGrams();

bool acquisitionSetTriggerGrams(float grams);

float acquisitionGetStopGrams();

#endif