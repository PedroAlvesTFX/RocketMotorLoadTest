/******************************************************************************
 *
 * Rocket Test Stand
 *
 * rts_wifi.h
 *
 * Gerenciamento da interface WiFi
 *
 ******************************************************************************/

#ifndef RTS_WIFI_H
#define RTS_WIFI_H

#include <Arduino.h>
#include <WiFi.h>

/******************************************************************************
 * Classe WiFiManager
 ******************************************************************************/

class WiFiManager
{
public:

    WiFiManager();

    //----------------------------------------------------------------------
    // Inicializa o WiFi
    //----------------------------------------------------------------------
    bool begin();

    //----------------------------------------------------------------------
    // Processamento periódico (reservado para futuras expansões)
    //----------------------------------------------------------------------
    void loop();

    //----------------------------------------------------------------------
    // Informações da rede
    //----------------------------------------------------------------------
    String getSSID() const;

    String getPassword() const;

    IPAddress getIP() const;

    bool isConnected() const;

    //----------------------------------------------------------------------
    // Reinicia o Access Point
    //----------------------------------------------------------------------
    bool restart();

private:

    const char *ssid;
    const char *password;

    IPAddress ip;
    IPAddress gateway;
    IPAddress subnet;
};

/******************************************************************************
 * Instância global
 ******************************************************************************/

extern WiFiManager wifiManager;

#endif