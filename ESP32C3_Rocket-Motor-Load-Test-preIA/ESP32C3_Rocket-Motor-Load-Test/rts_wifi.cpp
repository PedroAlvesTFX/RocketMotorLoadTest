/******************************************************************************
 *
 * Rocket Test Stand
 *
 * wifi.cpp
 *
 ******************************************************************************/

#include "rts_wifi.h"

WiFiManager wifiManager;

/******************************************************************************
 * Construtor
 ******************************************************************************/

WiFiManager::WiFiManager()
{
    ssid = "RocketTestStand";
    password = "rocket123";

    ip = IPAddress(192,168,4,1);
    gateway = IPAddress(192,168,4,1);
    subnet = IPAddress(255,255,255,0);
}

/******************************************************************************
 * Inicialização
 ******************************************************************************/

bool WiFiManager::begin()
{
    Serial.println();
    Serial.println("=========================================");
    Serial.println("Starting WiFi Access Point");
    Serial.println("=========================================");
    WiFi.setSleep(false);
    WiFi.mode(WIFI_AP);

    WiFi.softAPConfig(ip, gateway, subnet);

    if (!WiFi.softAP(ssid, password))
    {
        Serial.println("ERROR starting AP");

        return false;
    }
    Serial.print("WiFi Mode = ");
    Serial.println(WiFi.getMode());

    Serial.print("AP IP = ");
    Serial.println(WiFi.softAPIP());

    Serial.print("AP MAC = ");
    Serial.println(WiFi.softAPmacAddress());

    Serial.print("Canal = ");
    Serial.println(WiFi.channel());

    Serial.println();
    Serial.println("Access Point Started");

    Serial.print("SSID.....: ");
    Serial.println(ssid);

    Serial.print("Password.: ");
    Serial.println(password);

    Serial.print("IP.......: ");
    Serial.println(WiFi.softAPIP());

    Serial.print("Clients..: ");
    Serial.println(WiFi.softAPgetStationNum());

    return true;
}

/******************************************************************************
 * Loop
 ******************************************************************************/

void WiFiManager::loop()
{
    // Reservado para futuras expansões
}

/******************************************************************************
 * SSID
 ******************************************************************************/

String WiFiManager::getSSID() const
{
    return String(ssid);
}

/******************************************************************************
 * Password
 ******************************************************************************/

String WiFiManager::getPassword() const
{
    return String(password);
}

/******************************************************************************
 * IP
 ******************************************************************************/

IPAddress WiFiManager::getIP() const
{
    return WiFi.softAPIP();
}

/******************************************************************************
 * Conectado
 ******************************************************************************/

bool WiFiManager::isConnected() const
{
    return true;
}

/******************************************************************************
 * Restart
 ******************************************************************************/

bool WiFiManager::restart()
{
    WiFi.softAPdisconnect(true);

    delay(500);

    return begin();
}