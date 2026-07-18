#include "globals.h"

TestState state = STATE_IDLE;

bool campaignRequested = false;

char campaignDateTime[32] = "";
char campaignDescription[51] = "";

uint32_t currentCampaign = 1;