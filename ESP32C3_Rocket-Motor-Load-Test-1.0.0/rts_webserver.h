/******************************************************************************
 *
 * Rocket Motor Test Stand
 *
 * rts_webserver.h
 *
 ******************************************************************************/

#ifndef RTS_WEBSERVER_H
#define RTS_WEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>

class RocketWebServer
{
public:


    RocketWebServer();

    //----------------------------------------------------------
    // Inicialização
    //----------------------------------------------------------

    bool begin();

    void loop();

    //----------------------------------------------------------
    // Estado
    //----------------------------------------------------------

    bool isRunning() const;

private:

    //----------------------------------------------------------
    // Servidor HTTP
    //----------------------------------------------------------

    WebServer server;

    bool running;

    //----------------------------------------------------------
    // Rotas
    //----------------------------------------------------------

    void setupRoutes();

    //----------------------------------------------------------
    // Páginas
    //----------------------------------------------------------

    void handleRoot();

    void handleStatus();

    void handleStart();

    void handleStop();

    void handleTare();

    void handleCSV();

    void handleNotFound();

    //----------------------------------------------------------
    // Utilidades
    //----------------------------------------------------------

    String getStateString() const;

    String buildStatusJSON() const;
};

extern RocketWebServer webServer;

#endif