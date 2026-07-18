#include "rts_webserver.h"

#include <WiFi.h>

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
    server.on("/history", HTTP_GET, [this]() { handleHistory(); });
    server.on("/test", HTTP_GET, [this]() { handleTestPage(); });
    server.on("/campaign", HTTP_GET, [this]() { handleCampaignInfo(); });
    server.on("/campaign/data", HTTP_GET, [this]() { handleCampaignData(); });
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

    size_t csvSize = 0;
    if (!storage.getCSVSize(campaignId, csvSize) || csvSize == 0)
    {
        server.send(500, "text/plain; charset=utf-8", "Erro ao preparar o arquivo CSV");
        return;
    }

    String filename = "campaign" + String(campaignId) + ".csv";
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("Connection", "close");
    server.setContentLength(csvSize);
    server.send(200, "text/csv; charset=utf-8", "");

    WiFiClient client = server.client();
    client.setNoDelay(true);
    client.setTimeout(10000);

    Serial.print("CSV download started, campaign ");
    Serial.print(campaignId);
    Serial.print(", bytes: ");
    Serial.println(csvSize);

    bool sent = storage.exportCSV(campaignId, client);
    client.flush();

    Serial.println(sent ? "CSV download finished" : "CSV download failed");
}

void RocketWebServer::handleHistory()
{
    server.sendHeader("Cache-Control", "no-store");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json; charset=utf-8", "");

    server.sendContent("{\"campaigns\":[");

    uint32_t lastId = storage.getLastCampaignId();
    bool first = true;

    for (uint32_t id = lastId; id > 0; --id)
    {
        CampaignInfo info;
        if (!storage.getCampaignInfo(id, info))
            continue;

        String item;
        item.reserve(150);

        if (!first)
            item += ',';

        item += "{\"id\":";
        item += String(info.campaignId);
        item += ",\"datetime\":\"";

        for (size_t i = 0; info.dateTime[i] != '\0'; ++i)
        {
            char c = info.dateTime[i];
            if (c == '"' || c == '\\')
                item += '\\';
            item += c;
        }

        item += "\",\"samples\":";
        item += String(info.sampleCount);
        item += ",\"peak\":";
        item += String(info.peakForce, 1);
        item += '}';

        server.sendContent(item);
        first = false;
        delay(0);
    }

    server.sendContent("]}");
    server.sendContent("");
}


void RocketWebServer::handleTestPage()
{
    server.send_P(200, "text/html; charset=utf-8", TEST_HTML);
}

void RocketWebServer::handleCampaignInfo()
{
    if (!server.hasArg("id"))
    {
        server.send(400, "application/json", "{\"error\":\"missing id\"}");
        return;
    }

    uint32_t campaignId = (uint32_t)server.arg("id").toInt();
    CampaignStatistics statistics;
    if (campaignId == 0 || !storage.getCampaignStatistics(campaignId, statistics))
    {
        server.send(404, "application/json", "{\"error\":\"campaign not found\"}");
        return;
    }

    String json;
    json.reserve(320);
    json += "{\"id\":";
    json += String(statistics.campaignId);
    json += ",\"datetime\":\"";
    for (size_t i = 0; statistics.dateTime[i] != '\0'; ++i)
    {
        char c = statistics.dateTime[i];
        if (c == '"' || c == '\\')
            json += '\\';
        json += c;
    }
    json += "\",\"samples\":";
    json += String(statistics.sampleCount);
    json += ",\"peak\":";
    json += String(statistics.peakForce, 2);
    json += ",\"minimum\":";
    json += String(statistics.minimumForce, 2);
    json += ",\"average\":";
    json += String(statistics.averageForce, 2);
    json += ",\"impulse\":";
    json += String(statistics.impulseGramSeconds, 3);
    json += ",\"duration\":";
    json += String(statistics.durationSeconds, 3);
    json += "}";

    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json; charset=utf-8", json);
}

void RocketWebServer::handleCampaignData()
{
    if (!server.hasArg("id"))
    {
        server.send(400, "application/json", "{\"error\":\"missing id\"}");
        return;
    }

    uint32_t campaignId = (uint32_t)server.arg("id").toInt();
    if (campaignId == 0 || !storage.exists(campaignId))
    {
        server.send(404, "application/json", "{\"error\":\"campaign not found\"}");
        return;
    }

    size_t jsonSize = 0;
    if (!storage.getCampaignJSONSize(campaignId, jsonSize) || jsonSize == 0)
    {
        server.send(500, "application/json", "{\"error\":\"unable to prepare data\"}");
        return;
    }

    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("Connection", "close");
    server.setContentLength(jsonSize);
    server.send(200, "application/json; charset=utf-8", "");

    WiFiClient client = server.client();
    client.setNoDelay(true);
    client.setTimeout(10000);

    bool sent = storage.streamCampaignJSON(campaignId, client);
    client.flush();

    if (!sent)
        Serial.println("Campaign JSON transfer failed");
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
    json += String(storage.getLastCampaignId());
    json += ",\"campaignCount\":";
    json += String(storage.getCampaignCount());
    json += "}";
    return json;
}
