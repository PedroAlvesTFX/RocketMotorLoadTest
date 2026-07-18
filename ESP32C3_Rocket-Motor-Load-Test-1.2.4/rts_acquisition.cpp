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
static uint32_t belowStopSince = 0;
static uint32_t postTriggerStartMillis = 0;
static uint32_t testStartMillis = 0;
static uint32_t testStartMicros = 0;

static float effectiveStopThreshold()
{
    // Para motores pequenos, um trigger abaixo do limite padrão de parada
    // exige um limite de encerramento proporcionalmente menor.
    if (triggerGrams <= STOP_GRAMS)
        return max(MIN_TRIGGER_GRAMS, triggerGrams * 0.5f);

    return STOP_GRAMS;
}

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
    belowStopSince = 0;
    postTriggerStartMillis = 0;
    currentForce = 0.0f;
    peakForce = 0.0f;
    resetPreTrigger();

    Serial.print("Trigger.....: ");
    Serial.print(triggerGrams, 1);
    Serial.println(" g");
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
            if (currentForce >= triggerGrams)
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
                    postTriggerStartMillis = 0;
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

            const uint32_t nowMillis = millis();
            const uint32_t elapsedMillis = nowMillis - testStartMillis;

            // Depois que o fim da queima é confirmado, mantém a aquisição por
            // mais POST_TRIGGER_TIME_MS para registrar a linha de base final.
            if (postTriggerStartMillis != 0)
            {
                if ((nowMillis - postTriggerStartMillis) >= POST_TRIGGER_TIME_MS)
                {
                    Serial.println("Post-trigger recording finished");
                    campaign.stop();
                    state = STATE_SAVING;
                }
                else if (elapsedMillis >= (MAX_TEST_TIME_MS + POST_TRIGGER_TIME_MS))
                {
                    Serial.println("Maximum recording time including post-trigger reached");
                    campaign.stop();
                    state = STATE_SAVING;
                }

                break;
            }

            if (elapsedMillis >= MAX_TEST_TIME_MS)
            {
                Serial.println("Maximum test time reached");
                campaign.stop();
                state = STATE_SAVING;
                break;
            }

            if (currentForce <= effectiveStopThreshold())
            {
                if (belowStopSince == 0)
                    belowStopSince = nowMillis;

                if ((nowMillis - belowStopSince) >= STOP_TIME_MS)
                {
                    postTriggerStartMillis = nowMillis;

                    Serial.print("Burn end detected. Recording post-trigger for ");
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
    postTriggerStartMillis = 0;
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
        postTriggerStartMillis = 0;
        state = campaign.getCount() > 0 ? STATE_SAVING : STATE_IDLE;
    }
    else if (state == STATE_ARMED || state == STATE_WAIT_TRIGGER)
    {
        campaign.clear();
        postTriggerStartMillis = 0;
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

    size_t written = preferences.putFloat("trigger_g", grams);
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
