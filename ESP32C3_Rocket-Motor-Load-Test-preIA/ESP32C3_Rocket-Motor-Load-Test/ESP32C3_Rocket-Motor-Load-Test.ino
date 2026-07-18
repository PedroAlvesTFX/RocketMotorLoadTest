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
    Serial.println("Rocket Test Stand");
    Serial.println("ESP32-C3");
    Serial.println("----------------------------------");

    Serial.print("HX711 DOUT : ");
    Serial.println(HX711_DOUT_PIN);

    Serial.print("HX711 SCK  : ");
    Serial.println(HX711_SCK_PIN);

    // Inicializa armazenamento
    storage.begin();

    // Inicializa HX711
    acquisitionBegin();

    // Inicializa WiFi
    wifiManager.begin();

    // Inicializa WebServer
    webServer.begin();
    state = STATE_IDLE;

    Serial.println();
    Serial.println("System Ready");
}

void loop()
{
    wifiManager.loop();

    webServer.loop();

    acquisitionLoop();
    switch(state)
    {
        case STATE_IDLE:
            break;

        case STATE_ARMED:
            break;

        case STATE_WAIT_TRIGGER:
            break;

        case STATE_RECORDING:
            break;

        case STATE_SAVING:
            break;

        case STATE_FINISHED:
            state = STATE_IDLE;
            break;
    }
}