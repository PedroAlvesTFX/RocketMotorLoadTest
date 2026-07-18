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

#endif