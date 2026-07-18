#include "rts_webserver.h"

#include "globals.h"
#include "campaign.h"
#include "rts_storage.h"
#include "rts_acquisition.h"
#include "rts_clock.h"
#include "index.h"

RocketWebServer webServer;

RocketWebServer::RocketWebServer() : server(80), running(false)
{
}

bool RocketWebServer::begin()
{
    setupRoutes();
    server.begin();
    running = true;
    Serial.println("HTTP Server Started");
    return true;
}

void RocketWebServer::loop()
{
    if (running)
        server.handleClient();
}

bool RocketWebServer::isRunning() const
{
    return running;
}

void RocketWebServer::setupRoutes()
{
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/start", HTTP_POST, [this]() { handleStart(); });
    server.on("/stop", HTTP_POST, [this]() { handleStop(); });
    server.on("/tare", HTTP_POST, [this]() { handleTare(); });
    server.on("/csv", HTTP_GET, [this]() { handleCSV(); });
    server.onNotFound([this]() { handleNotFound(); });
}

void RocketWebServer::handleRoot()
{
    server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void RocketWebServer::handleStatus()
{
    server.send(200, "application/json", buildStatusJSON());
}

void RocketWebServer::handleStart()
{
    String body = server.arg("plain");
    String dateTime;

    int key = body.indexOf("\"datetime\"");
    if (key >= 0)
    {
        int colon = body.indexOf(':', key);
        int firstQuote = body.indexOf('"', colon + 1);
        int secondQuote = body.indexOf('"', firstQuote + 1);

        if (firstQuote >= 0 && secondQuote > firstQuote)
            dateTime = body.substring(firstQuote + 1, secondQuote);
    }

    if (dateTime.length() > 0)
    {
        rtcClock.setDateTime(dateTime);
        dateTime.toCharArray(campaignDateTime, sizeof(campaignDateTime));
    }
    else
    {
        strncpy(campaignDateTime, "unknown", sizeof(campaignDateTime) - 1);
        campaignDateTime[sizeof(campaignDateTime) - 1] = '\0';
    }

    if (!acquisitionStart())
    {
        server.send(409, "text/plain; charset=utf-8", "O sistema não está disponível para iniciar outro ensaio");
        return;
    }

    Serial.println("Campaign armed from WEB");
    server.send(200, "text/plain; charset=utf-8", "Ensaio armado. Aguardando o disparo.");
}

void RocketWebServer::handleStop()
{
    acquisitionStop();
    server.send(200, "text/plain; charset=utf-8", "Ensaio interrompido");
}

void RocketWebServer::handleTare()
{
    if (state != STATE_IDLE)
    {
        server.send(409, "text/plain; charset=utf-8", "A tara só pode ser executada em IDLE");
        return;
    }

    acquisitionTare();
    server.send(200, "text/plain; charset=utf-8", "Tara executada");
}

void RocketWebServer::handleCSV()
{
    if (!server.hasArg("id"))
    {
        server.send(400, "text/plain", "Missing campaign id");
        return;
    }

    uint32_t campaignId = (uint32_t)server.arg("id").toInt();
    if (campaignId == 0 || !storage.exists(campaignId))
    {
        server.send(404, "text/plain", "Campaign not found");
        return;
    }

    String filename = "campaign" + String(campaignId) + ".csv";
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/csv; charset=utf-8", "");
    storage.exportCSV(campaignId, server.client());
}

void RocketWebServer::handleNotFound()
{
    server.send(404, "text/plain", "404 - Not Found");
}

String RocketWebServer::getStateString() const
{
    switch (state)
    {
        case STATE_IDLE: return "IDLE";
        case STATE_ARMED: return "ARMED";
        case STATE_WAIT_TRIGGER: return "WAIT_TRIGGER";
        case STATE_RECORDING: return "RECORDING";
        case STATE_SAVING: return "SAVING";
        case STATE_FINISHED: return "FINISHED";
        default: return "UNKNOWN";
    }
}

String RocketWebServer::buildStatusJSON() const
{
    String json;
    json.reserve(160);
    json += "{\"state\":\"";
    json += getStateString();
    json += "\",\"force\":";
    json += String(acquisitionGetForce(), 1);
    json += ",\"peak\":";
    json += String(acquisitionGetPeak(), 1);
    json += ",\"recording\":";
    json += acquisitionIsRecording() ? "true" : "false";
    json += ",\"lastCampaign\":";
    json += String(currentCampaign);
    json += ",\"campaignCount\":";
    json += String(storage.getCampaignCount());
    json += "}";
    return json;
}
