#include "rts_acquisition.h"

#include "config.h"
#include "globals.h"
#include "hx711.h"
#include "campaign.h"
#include "rts_storage.h"

static float currentForce = 0.0f;
static float peakForce = 0.0f;

static uint8_t triggerCounter = 0;
static uint32_t belowStopSince = 0;
static uint32_t testStartMillis = 0;
static uint32_t testStartMicros = 0;

#define PRE_TRIGGER_SAMPLES (SAMPLE_RATE * PRE_TRIGGER_SECONDS)

static Sample preTrigger[PRE_TRIGGER_SAMPLES];
static uint16_t preTriggerCount = 0;
static uint16_t preTriggerWrite = 0;

static void resetPreTrigger()
{
    preTriggerCount = 0;
    preTriggerWrite = 0;
}

static void addPreTriggerSample(float grams)
{
    preTrigger[preTriggerWrite].us = micros();
    preTrigger[preTriggerWrite].grams = grams;

    preTriggerWrite = (preTriggerWrite + 1) % PRE_TRIGGER_SAMPLES;

    if (preTriggerCount < PRE_TRIGGER_SAMPLES)
        preTriggerCount++;
}

static void copyPreTriggerToCampaign()
{
    if (preTriggerCount == 0)
        return;

    uint16_t oldest = (preTriggerCount == PRE_TRIGGER_SAMPLES)
                        ? preTriggerWrite
                        : 0;

    uint32_t firstMicros = preTrigger[oldest].us;

    for (uint16_t i = 0; i < preTriggerCount; i++)
    {
        uint16_t index = (oldest + i) % PRE_TRIGGER_SAMPLES;
        campaign.addSample(preTrigger[index].us - firstMicros,
                           preTrigger[index].grams);
    }

    testStartMicros = firstMicros;
}

void acquisitionBegin()
{
    Scale.begin();
    campaign.begin();

    state = STATE_IDLE;
    triggerCounter = 0;
    belowStopSince = 0;
    currentForce = 0.0f;
    peakForce = 0.0f;
    resetPreTrigger();
}

void acquisitionLoop()
{
    if (!Scale.update())
        return;

    currentForce = Scale.getGrams();

    if (state == STATE_ARMED)
        addPreTriggerSample(currentForce);

    switch (state)
    {
        case STATE_IDLE:
            break;

        case STATE_ARMED:
            if (currentForce >= TRIGGER_GRAMS)
            {
                triggerCounter++;

                if (triggerCounter >= 3)
                {
                    Serial.println();
                    Serial.println("==================================");
                    Serial.println("TEST START");
                    Serial.println("==================================");

                    campaign.start();
                    copyPreTriggerToCampaign();

                    peakForce = currentForce;
                    triggerCounter = 0;
                    belowStopSince = 0;
                    testStartMillis = millis();

                    state = STATE_RECORDING;
                }
            }
            else
            {
                triggerCounter = 0;
            }
            break;

        case STATE_WAIT_TRIGGER:
            // Mantido por compatibilidade com a máquina de estados original.
            state = STATE_ARMED;
            break;

        case STATE_RECORDING:
        {
            uint32_t elapsedUS = micros() - testStartMicros;

            if (!campaign.addSample(elapsedUS, currentForce))
            {
                Serial.println("Campaign sample buffer full");
                campaign.stop();
                state = STATE_SAVING;
                break;
            }

            if (currentForce > peakForce)
                peakForce = currentForce;

            if ((millis() - testStartMillis) >= MAX_TEST_TIME_MS)
            {
                Serial.println("Maximum test time reached");
                campaign.stop();
                state = STATE_SAVING;
                break;
            }

            if (fabsf(currentForce) < STOP_GRAMS)
            {
                if (belowStopSince == 0)
                    belowStopSince = millis();

                if ((millis() - belowStopSince) >= STOP_TIME_MS)
                {
                    Serial.println("Burn finished");
                    campaign.stop();
                    state = STATE_SAVING;
                }
            }
            else
            {
                belowStopSince = 0;
            }
            break;
        }

        case STATE_SAVING:
        {
            Serial.println();
            Serial.println("Saving campaign...");

            currentCampaign = storage.nextCampaignId();

            bool saved = storage.saveCampaign(
                currentCampaign,
                campaignDateTime,
                campaignDescription,
                campaign);

            Serial.print("Samples : ");
            Serial.println(campaign.getCount());
            Serial.print("Peak    : ");
            Serial.print(campaign.getPeak());
            Serial.println(" g");

            state = saved ? STATE_FINISHED : STATE_IDLE;
            break;
        }

        case STATE_FINISHED:
            state = STATE_IDLE;
            break;

        default:
            state = STATE_IDLE;
            break;
    }
}

float acquisitionGetForce()
{
    return currentForce;
}

float acquisitionGetPeak()
{
    return peakForce;
}

bool acquisitionIsRecording()
{
    return state == STATE_RECORDING;
}

bool acquisitionStart()
{
    if (state != STATE_IDLE)
        return false;

    peakForce = 0.0f;
    triggerCounter = 0;
    belowStopSince = 0;
    campaign.clear();
    resetPreTrigger();

    state = STATE_ARMED;
    return true;
}

void acquisitionStop()
{
    if (state == STATE_RECORDING)
    {
        campaign.stop();
        state = campaign.getCount() > 0 ? STATE_SAVING : STATE_IDLE;
    }
    else if (state == STATE_ARMED || state == STATE_WAIT_TRIGGER)
    {
        campaign.clear();
        resetPreTrigger();
        state = STATE_IDLE;
    }
}

void acquisitionTare()
{
    if (state == STATE_IDLE)
    {
        Scale.tare();
        peakForce = 0.0f;
    }
}
