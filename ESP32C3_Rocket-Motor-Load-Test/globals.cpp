#include "globals.h"
#include "config.h"

TestState state = STATE_IDLE;

bool campaignRequested = false;

char campaignDateTime[32] = "";
char campaignDescription[51] = "";

uint32_t currentCampaign = 1;

float triggerGrams = DEFAULT_TRIGGER_GRAMS;