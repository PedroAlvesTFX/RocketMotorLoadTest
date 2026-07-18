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
    bool importUploadFailed;

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

    void handleRTS();

    void handleImportCSV();

    void handleImportRTS();

    void handleImportRTSUpload();

    void handleHistory();

    void handleTestPage();

    void handleCampaignInfo();

    void handleCampaignData();

    void handleDeleteCampaign();

    void handleUpdateDescription();

    void handleNotFound();

    //----------------------------------------------------------
    // Utilidades
    //----------------------------------------------------------

    String getStateString() const;

    String buildStatusJSON() const;
};

extern RocketWebServer webServer;

#endif