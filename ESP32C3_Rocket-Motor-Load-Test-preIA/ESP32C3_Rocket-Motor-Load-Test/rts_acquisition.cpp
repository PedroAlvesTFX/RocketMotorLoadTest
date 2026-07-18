#include "rts_acquisition.h"

#include "config.h"
#include "globals.h"
#include "hx711.h"
#include "campaign.h"
#include "rts_storage.h"

static float currentForce = 0.0f;
static float peakForce = 0.0f;

static uint8_t triggerCounter = 0;
static uint16_t stopCounter = 0;

static uint32_t testStartMillis = 0;
static uint32_t testStartMicros = 0;

/************************************************************/

void acquisitionBegin()
{
    Scale.begin();

    state = STATE_IDLE;

    triggerCounter = 0;
    stopCounter = 0;

    currentForce = 0;
    peakForce = 0;
}

/************************************************************/

void acquisitionLoop()
{
    //-------------------------------------------------------
    // Atualiza HX711
    //-------------------------------------------------------

    if (!Scale.update())
        return;

    currentForce = Scale.getGrams();

    if (currentForce > peakForce)
        peakForce = currentForce;

    //-------------------------------------------------------
    // Máquina de estados
    //-------------------------------------------------------

    switch (state)
    {

    //-------------------------------------------------------
    case STATE_IDLE:
        break;

    //-------------------------------------------------------
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

                campaign.clear();
                campaign.start();

                peakForce = currentForce;

                triggerCounter = 0;
                stopCounter = 0;

                testStartMillis = millis();
                testStartMicros = micros();

                state = STATE_RECORDING;
            }
        }
        else
        {
            triggerCounter = 0;
        }

        break;

    //-------------------------------------------------------
    case STATE_RECORDING:
    {
        uint32_t elapsedUS = micros() - testStartMicros;

        campaign.addSample(elapsedUS, currentForce);

        if ((millis() - testStartMillis) >= MAX_TEST_TIME_MS)
        {
            Serial.println("Maximum test time reached");

            campaign.stop();

            state = STATE_SAVING;

            break;
        }

        if (currentForce < STOP_GRAMS)
        {
            stopCounter++;

            if (stopCounter >= 40)
            {
                Serial.println();
                Serial.println("Burn finished");

                campaign.stop();

                state = STATE_SAVING;
            }
        }
        else
        {
            stopCounter = 0;
        }
    }
    break;

    //-------------------------------------------------------
    case STATE_SAVING:

        Serial.println();
        Serial.println("Saving campaign...");

        storage.saveCampaign(
            storage.nextCampaignId(),
            campaignDateTime,
            campaign);

        Serial.print("Samples : ");
        Serial.println(campaign.getCount());

        Serial.print("Peak    : ");
        Serial.print(campaign.getPeak());
        Serial.println(" g");

        state = STATE_FINISHED;

        break;

    //-------------------------------------------------------
    case STATE_FINISHED:

        Serial.println();
        Serial.println("Campaign Finished");

        state = STATE_IDLE;

        break;

    default:
        break;
    }
}

/************************************************************/

float acquisitionGetForce()
{
    return currentForce;
}

/************************************************************/

float acquisitionGetPeak()
{
    return peakForce;
}

/************************************************************/

bool acquisitionIsRecording()
{
    return (state == STATE_RECORDING);
}

/************************************************************/

bool acquisitionStart()
{
    if (state != STATE_IDLE)
        return false;

    peakForce = 0;
    triggerCounter = 0;
    stopCounter = 0;

    state = STATE_ARMED;

    return true;
}

/************************************************************/

void acquisitionStop()
{
    state = STATE_FINISHED;
}

/************************************************************/

void acquisitionTare()
{
    Scale.tare();
}