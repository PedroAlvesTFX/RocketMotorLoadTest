#include "rts_wifi.h"
#include "config.h"

WiFiManager wifiManager;

WiFiManager::WiFiManager()
{
    ssid = AP_SSID;
    password = AP_PASSWORD;
    ip = IPAddress(192, 168, 4, 1);
    gateway = IPAddress(192, 168, 4, 1);
    subnet = IPAddress(255, 255, 255, 0);
}

bool WiFiManager::begin()
{
    Serial.println();
    Serial.println("=========================================");
    Serial.println("Starting WiFi Access Point");
    Serial.println("=========================================");

    WiFi.setSleep(false);
    WiFi.mode(WIFI_AP);

    if (!WiFi.softAPConfig(ip, gateway, subnet))
    {
        Serial.println("ERROR configuring AP network");
        return false;
    }

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
    Serial.print("Channel = ");
    Serial.println(WiFi.channel());
    Serial.println("Access Point Started");
    Serial.print("SSID.....: ");
    Serial.println(ssid);
    Serial.print("Password.: ");
    Serial.println(password);
    Serial.print("IP.......: ");
    Serial.println(WiFi.softAPIP());

    return true;
}

void WiFiManager::loop()
{
}

String WiFiManager::getSSID() const
{
    return String(ssid);
}

String WiFiManager::getPassword() const
{
    return String(password);
}

IPAddress WiFiManager::getIP() const
{
    return WiFi.softAPIP();
}

bool WiFiManager::isConnected() const
{
    return WiFi.getMode() == WIFI_AP && WiFi.softAPIP() == ip;
}

bool WiFiManager::restart()
{
    WiFi.softAPdisconnect(true);
    delay(500);
    return begin();
}
