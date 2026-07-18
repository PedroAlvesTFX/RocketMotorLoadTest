#include <Arduino.h>

#include "config.h"
#include "types.h"
#include "globals.h"
#include "hx711.h"
#include "rts_acquisition.h"
#include "rts_storage.h"
#include "rts_wifi.h"
#include "rts_webserver.h"
#include "rts_clock.h"

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("----------------------------------");
    Serial.println("Rocket Motor Test Stand");
    Serial.println("Release 1.0");
    Serial.println("----------------------------------");
    Serial.print("HX711 DOUT : ");
    Serial.println(HX711_DOUT_PIN);
    Serial.print("HX711 SCK  : ");
    Serial.println(HX711_SCK_PIN);

    if (!storage.begin())
        Serial.println("WARNING: storage unavailable");

    acquisitionBegin();

    if (!wifiManager.begin())
        Serial.println("WARNING: WiFi AP unavailable");

    if (!webServer.begin())
        Serial.println("WARNING: web server unavailable");

    state = STATE_IDLE;

    Serial.println();
    Serial.println("System Ready");
    Serial.println("Open http://192.168.4.1");
}

void loop()
{
    wifiManager.loop();
    webServer.loop();
    acquisitionLoop();
    delay(0);
}
