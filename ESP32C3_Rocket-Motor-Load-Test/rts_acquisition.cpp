#include "rts_acquisition.h"

#include "config.h"
#include "globals.h"
#include "hx711.h"
#include "campaign.h"
#include "rts_storage.h"

#include <Preferences.h>

static float currentForce = 0.0f;
static float peakForce = 0.0f;

static uint8_t triggerCounter = 0;
static uint32_t triggerCandidateMicros = 0;
static uint32_t triggerConfirmedMillis = 0;
static uint32_t belowStopSince = 0;
static uint32_t postTriggerStartMillis = 0;
static uint32_t campaignTimelineStartMicros = 0;

// Buffer circular de pré-trigger. Sua duração é limitada pelo tempo real,
// e não pela quantidade nominal de amostras.
static Sample preTrigger[PRE_TRIGGER_BUFFER_SAMPLES];
static uint16_t preTriggerOldest = 0;
static uint16_t preTriggerCount = 0;

static float effectiveStopThreshold()
{
    if (triggerGrams <= STOP_GRAMS)
        return max(MIN_TRIGGER_GRAMS, triggerGrams * 0.5f);

    return STOP_GRAMS;
}

static void resetPreTrigger()
{
    preTriggerOldest = 0;
    preTriggerCount = 0;
}

static uint16_t preTriggerPhysicalIndex(uint16_t logicalIndex)
{
    return (uint16_t)((preTriggerOldest + logicalIndex) % PRE_TRIGGER_BUFFER_SAMPLES);
}

static void removeOldestPreTrigger()
{
    if (preTriggerCount == 0)
        return;

    preTriggerOldest = (uint16_t)((preTriggerOldest + 1) % PRE_TRIGGER_BUFFER_SAMPLES);
    preTriggerCount--;
}

static void addPreTriggerSample(uint32_t sampleMicros, float grams)
{
    // Se a capacidade física for atingida, preserva as amostras mais novas.
    if (preTriggerCount == PRE_TRIGGER_BUFFER_SAMPLES)
        removeOldestPreTrigger();

    const uint16_t writeIndex = preTriggerPhysicalIndex(preTriggerCount);
    preTrigger[writeIndex].us = sampleMicros;
    preTrigger[writeIndex].grams = grams;
    preTriggerCount++;

    const uint32_t windowUs = (uint32_t)PRE_TRIGGER_SECONDS * 1000000UL;

    // Remove tudo que estiver há mais de PRE_TRIGGER_SECONDS da leitura atual.
    while (preTriggerCount > 1)
    {
        const Sample &oldest = preTrigger[preTriggerOldest];
        if ((uint32_t)(sampleMicros - oldest.us) <= windowUs)
            break;

        removeOldestPreTrigger();
    }
}

static bool copyPreTriggerToCampaign(uint32_t triggerMicros)
{
    if (preTriggerCount == 0)
        return false;

    const uint32_t windowUs = (uint32_t)PRE_TRIGGER_SECONDS * 1000000UL;

    // Encontra a primeira leitura dentro da janela exata de dois segundos
    // anterior ao primeiro ponto que iniciou a confirmação do trigger.
    uint16_t firstLogical = 0;
    while (firstLogical + 1 < preTriggerCount)
    {
        const Sample &sample = preTrigger[preTriggerPhysicalIndex(firstLogical)];
        if ((uint32_t)(triggerMicros - sample.us) <= windowUs)
            break;
        firstLogical++;
    }

    const Sample &first = preTrigger[preTriggerPhysicalIndex(firstLogical)];
    campaignTimelineStartMicros = first.us;

    for (uint16_t logical = firstLogical; logical < preTriggerCount; logical++)
    {
        const Sample &sample = preTrigger[preTriggerPhysicalIndex(logical)];
        const uint32_t relativeUs = (uint32_t)(sample.us - campaignTimelineStartMicros);

        if (!campaign.addSample(relativeUs, sample.grams))
            return false;
    }

    return campaign.getCount() > 0;
}

static void finishCapture(const __FlashStringHelper *reason)
{
    Serial.println(reason);
    campaign.stop();
    state = STATE_PROCESSING;
}

void acquisitionBegin()
{
    Preferences preferences;
    if (preferences.begin("rocket-test", true))
    {
        triggerGrams = preferences.getFloat("trigger_g", DEFAULT_TRIGGER_GRAMS);
        preferences.end();
    }

    if (!isfinite(triggerGrams) ||
        triggerGrams < MIN_TRIGGER_GRAMS ||
        triggerGrams > MAX_TRIGGER_GRAMS)
    {
        triggerGrams = DEFAULT_TRIGGER_GRAMS;
    }

    Scale.begin();
    campaign.begin();

    state = STATE_IDLE;
    triggerCounter = 0;
    triggerCandidateMicros = 0;
    triggerConfirmedMillis = 0;
    belowStopSince = 0;
    postTriggerStartMillis = 0;
    campaignTimelineStartMicros = 0;
    currentForce = 0.0f;
    peakForce = 0.0f;
    resetPreTrigger();

    Serial.print("Trigger.....: ");
    Serial.print(triggerGrams, 1);
    Serial.println(" g");
}

void acquisitionLoop()
{
    // PROCESSING e SAVING não dependem de uma nova conversão do HX711.
    if (state == STATE_PROCESSING)
    {
        state = STATE_SAVING;
        return;
    }

    if (state == STATE_SAVING)
    {
        Serial.println();
        Serial.println("Saving campaign...");

        currentCampaign = storage.nextCampaignId();
        const bool saved = storage.saveCampaign(
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
        return;
    }

    if (state == STATE_FINISHED)
    {
        state = STATE_IDLE;
        return;
    }

    if (!Scale.update())
        return;

    const uint32_t sampleMicros = micros();
    currentForce = Scale.getGrams();

    switch (state)
    {
        case STATE_IDLE:
            break;

        case STATE_WAIT_TRIGGER:
            // Compatibilidade com comandos/estados antigos.
            state = STATE_ARMED;
            break;

        case STATE_ARMED:
        {
            // A aquisição começa imediatamente ao armar.
            addPreTriggerSample(sampleMicros, currentForce);

            if (currentForce >= triggerGrams)
            {
                if (triggerCounter == 0)
                    triggerCandidateMicros = sampleMicros;

                triggerCounter++;

                if (triggerCounter >= 3)
                {
                    Serial.println();
                    Serial.println("==================================");
                    Serial.println("TRIGGER CONFIRMED");
                    Serial.println("==================================");

                    const uint32_t confirmedTriggerMicros = triggerCandidateMicros;

                    campaign.start();
                    if (!copyPreTriggerToCampaign(confirmedTriggerMicros))
                    {
                        Serial.println("Unable to copy pre-trigger buffer");
                        campaign.stop();
                        state = STATE_IDLE;
                        break;
                    }

                    peakForce = campaign.getPeak();
                    triggerCounter = 0;
                    triggerCandidateMicros = 0;
                    triggerConfirmedMillis = millis();
                    belowStopSince = 0;
                    postTriggerStartMillis = 0;
                    state = STATE_RECORDING;

                    const uint32_t triggerRelativeUs =
                        (uint32_t)(confirmedTriggerMicros - campaignTimelineStartMicros);

                    Serial.print("Pre-trigger captured: ");
                    Serial.print(campaign.getCount());
                    Serial.print(" samples, approximately ");
                    Serial.print((float)triggerRelativeUs / 1000000.0f, 3);
                    Serial.println(" s");
                }
            }
            else
            {
                triggerCounter = 0;
                triggerCandidateMicros = 0;
            }
            break;
        }

        case STATE_RECORDING:
        {
            const uint32_t elapsedUs = (uint32_t)(sampleMicros - campaignTimelineStartMicros);
            if (!campaign.addSample(elapsedUs, currentForce))
            {
                finishCapture(F("Campaign sample buffer full"));
                break;
            }

            if (currentForce > peakForce)
                peakForce = currentForce;

            const uint32_t nowMillis = millis();
            if ((nowMillis - triggerConfirmedMillis) >= MAX_TEST_TIME_MS)
            {
                finishCapture(F("Maximum test time reached"));
                break;
            }

            if (currentForce <= effectiveStopThreshold())
            {
                if (belowStopSince == 0)
                    belowStopSince = nowMillis;

                if ((nowMillis - belowStopSince) >= STOP_TIME_MS)
                {
                    postTriggerStartMillis = nowMillis;
                    state = STATE_POST_RECORDING;

                    Serial.print("Burn end confirmed. Recording baseline for ");
                    Serial.print(POST_TRIGGER_TIME_MS);
                    Serial.println(" ms");
                }
            }
            else
            {
                belowStopSince = 0;
            }
            break;
        }

        case STATE_POST_RECORDING:
        {
            const uint32_t elapsedUs = (uint32_t)(sampleMicros - campaignTimelineStartMicros);
            if (!campaign.addSample(elapsedUs, currentForce))
            {
                finishCapture(F("Campaign sample buffer full during post-trigger"));
                break;
            }

            if ((millis() - postTriggerStartMillis) >= POST_TRIGGER_TIME_MS)
                finishCapture(F("Post-trigger recording finished"));
            break;
        }

        default:
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
    return state == STATE_RECORDING || state == STATE_POST_RECORDING;
}

bool acquisitionIsBusy()
{
    return state == STATE_ARMED ||
           state == STATE_WAIT_TRIGGER ||
           state == STATE_RECORDING ||
           state == STATE_POST_RECORDING ||
           state == STATE_PROCESSING ||
           state == STATE_SAVING;
}

bool acquisitionStart()
{
    if (state != STATE_IDLE)
        return false;

    peakForce = 0.0f;
    triggerCounter = 0;
    triggerCandidateMicros = 0;
    triggerConfirmedMillis = 0;
    belowStopSince = 0;
    postTriggerStartMillis = 0;
    campaignTimelineStartMicros = 0;
    campaign.clear();
    resetPreTrigger();

    state = STATE_ARMED;
    return true;
}

void acquisitionStop()
{
    if (state == STATE_RECORDING || state == STATE_POST_RECORDING)
    {
        campaign.stop();
        state = campaign.getCount() > 0 ? STATE_PROCESSING : STATE_IDLE;
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

float acquisitionGetTriggerGrams()
{
    return triggerGrams;
}

bool acquisitionSetTriggerGrams(float grams)
{
    if (!isfinite(grams) || grams < MIN_TRIGGER_GRAMS || grams > MAX_TRIGGER_GRAMS)
        return false;

    Preferences preferences;
    if (!preferences.begin("rocket-test", false))
        return false;

    const size_t written = preferences.putFloat("trigger_g", grams);
    preferences.end();

    if (written == 0)
        return false;

    triggerGrams = grams;

    Serial.print("Trigger changed to: ");
    Serial.print(triggerGrams, 1);
    Serial.println(" g");

    return true;
}

float acquisitionGetStopGrams()
{
    return effectiveStopThreshold();
}
