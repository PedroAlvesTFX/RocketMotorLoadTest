#include "rts_webserver.h"

#include <WiFi.h>

#include "globals.h"
#include "campaign.h"
#include "rts_storage.h"
#include "rts_acquisition.h"
#include "hx711.h"
#include "rts_clock.h"
#include "index.h"

RocketWebServer webServer;

static String extractJsonString(const String &json, const char *key)
{
    String token = String('"') + key + '"';
    int keyPos = json.indexOf(token);
    if (keyPos < 0)
        return String();

    int colon = json.indexOf(':', keyPos + token.length());
    int quote = json.indexOf('"', colon + 1);
    if (colon < 0 || quote < 0)
        return String();

    String value;
    bool escaped = false;
    for (int i = quote + 1; i < (int)json.length(); ++i)
    {
        char c = json[i];
        if (escaped)
        {
            if (c == 'n') value += '\n';
            else if (c == 'r') value += '\r';
            else if (c == 't') value += '\t';
            else value += c;
            escaped = false;
        }
        else if (c == '\\')
        {
            escaped = true;
        }
        else if (c == '"')
        {
            break;
        }
        else
        {
            value += c;
        }
    }

    return value;
}


static bool extractJsonBool(const String &json, const char *key, bool &value)
{
    String token = String('"') + key + '"';
    int keyPos = json.indexOf(token);
    if (keyPos < 0)
        return false;

    int colon = json.indexOf(':', keyPos + token.length());
    if (colon < 0)
        return false;

    int pos = colon + 1;
    while (pos < (int)json.length() && isspace((unsigned char)json[pos]))
        pos++;

    if (json.startsWith("true", pos))
    {
        value = true;
        return true;
    }

    if (json.startsWith("false", pos))
    {
        value = false;
        return true;
    }

    return false;
}


static bool extractJsonFloat(const String &json, const char *key, float &value)
{
    String token = String('"') + key + '"';
    int keyPos = json.indexOf(token);
    if (keyPos < 0)
        return false;

    int colon = json.indexOf(':', keyPos + token.length());
    if (colon < 0)
        return false;

    int start = colon + 1;
    while (start < (int)json.length() && isspace((unsigned char)json[start]))
        start++;

    int end = start;
    while (end < (int)json.length())
    {
        char c = json[end];
        if (!(isdigit((unsigned char)c) || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E'))
            break;
        end++;
    }

    if (end <= start)
        return false;

    value = json.substring(start, end).toFloat();
    return isfinite(value);
}

static void appendJsonEscaped(String &target, const char *value)
{
    if (value == nullptr)
        return;

    for (size_t i = 0; value[i] != '\0'; ++i)
    {
        char c = value[i];
        if (c == '"' || c == '\\')
            target += '\\';
        else if (c == '\n' || c == '\r')
            c = ' ';
        target += c;
    }
}

RocketWebServer::RocketWebServer() : server(80), running(false), importUploadFailed(false)
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
    server.on("/rts", HTTP_GET, [this]() { handleRTS(); });
    server.on("/campaign/import-csv", HTTP_POST, [this]() { handleImportCSV(); });
    server.on("/campaign/import-rts", HTTP_POST,
              [this]() { handleImportRTS(); },
              [this]() { handleImportRTSUpload(); });
    server.on("/history", HTTP_GET, [this]() { handleHistory(); });
    server.on("/test", HTTP_GET, [this]() { handleTestPage(); });
    server.on("/campaign", HTTP_GET, [this]() { handleCampaignInfo(); });
    server.on("/campaign/data", HTTP_GET, [this]() { handleCampaignData(); });
    server.on("/campaign/delete", HTTP_POST, [this]() { handleDeleteCampaign(); });
    server.on("/campaign/description", HTTP_POST, [this]() { handleUpdateDescription(); });
    server.on("/settings/force-sign", HTTP_POST, [this]() { handleForceSign(); });
    server.on("/settings/trigger", HTTP_POST, [this]() { handleTriggerSetting(); });
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
    String dateTime = extractJsonString(body, "datetime");
    String description = extractJsonString(body, "description");

    description.trim();
    if (description.length() > 50)
        description.remove(50);

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

    description.toCharArray(campaignDescription, sizeof(campaignDescription));

    if (!acquisitionStart())
    {
        server.send(409, "text/plain; charset=utf-8", "O sistema não está disponível para iniciar outro ensaio");
        return;
    }

    Serial.print("Campaign armed from WEB: ");
    Serial.println(campaignDescription);
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


void RocketWebServer::handleRTS()
{
    if (!server.hasArg("id"))
    {
        server.send(400, "text/plain", "Missing campaign id");
        return;
    }

    uint32_t campaignId = (uint32_t)server.arg("id").toInt();
    size_t rtsSize = 0;
    if (campaignId == 0 || !storage.getRTSSize(campaignId, rtsSize))
    {
        server.send(404, "text/plain", "Campaign not found");
        return;
    }

    String filename = "campaign" + String(campaignId) + ".rts";
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("Connection", "close");
    server.setContentLength(rtsSize);
    server.send(200, "application/octet-stream", "");

    WiFiClient client = server.client();
    client.setNoDelay(true);
    client.setTimeout(10000);
    storage.exportRTS(campaignId, client);
    client.flush();
}

void RocketWebServer::handleImportCSV()
{
    if (state != STATE_IDLE)
    {
        server.send(409, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"A importação só pode ser feita em IDLE\"}");
        return;
    }

    uint32_t newId = 0;
    if (!storage.importCSV(server.arg("plain"), newId))
    {
        server.send(400, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"CSV inválido ou sem espaço disponível\"}");
        return;
    }

    String json = "{\"ok\":true,\"message\":\"Curva importada\",\"id\":";
    json += String(newId);
    json += "}";
    server.send(200, "application/json; charset=utf-8", json);
}

void RocketWebServer::handleImportRTSUpload()
{
    HTTPUpload &upload = server.upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        importUploadFailed = state != STATE_IDLE || !storage.beginRTSImport(0);
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (!importUploadFailed &&
            !storage.writeRTSImport(upload.buf, upload.currentSize))
        {
            importUploadFailed = true;
            storage.cancelRTSImport();
        }
    }
    else if (upload.status == UPLOAD_FILE_ABORTED)
    {
        importUploadFailed = true;
        storage.cancelRTSImport();
    }
}

void RocketWebServer::handleImportRTS()
{
    uint32_t newId = 0;
    if (importUploadFailed || !storage.finishRTSImport(newId))
    {
        storage.cancelRTSImport();
        server.send(400, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"Backup RTS inválido ou sem espaço disponível\"}");
        return;
    }

    String json = "{\"ok\":true,\"message\":\"Backup restaurado\",\"id\":";
    json += String(newId);
    json += "}";
    server.send(200, "application/json; charset=utf-8", json);
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

        item += "\",\"description\":\"";
        appendJsonEscaped(item, info.description);
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
    json += "\",\"description\":\"";
    appendJsonEscaped(json, statistics.description);
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
    json += ",\"ignitionTime\":";
    json += String(statistics.ignitionTimeSeconds, 3);
    json += ",\"peakTime\":";
    json += String(statistics.peakTimeSeconds, 3);
    json += ",\"timeToPeak\":";
    json += String(statistics.timeToPeakSeconds, 3);
    json += ",\"burnTime\":";
    json += String(statistics.burnTimeSeconds, 3);
    json += ",\"burnAverage\":";
    json += String(statistics.burnAverageForce, 2);
    json += ",\"sampleRate\":";
    json += String(statistics.sampleRateSps, 2);
    json += ",\"maxRiseRate\":";
    json += String(statistics.maxRiseRateGps, 2);
    json += ",\"ignitionIndex\":";
    json += String(statistics.ignitionIndex);
    json += ",\"peakIndex\":";
    json += String(statistics.peakIndex);
    json += ",\"burnEndIndex\":";
    json += String(statistics.burnEndIndex);
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


void RocketWebServer::handleUpdateDescription()
{
    if (state != STATE_IDLE)
    {
        server.send(409, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"A descrição só pode ser alterada em IDLE\"}");
        return;
    }

    if (!server.hasArg("id"))
    {
        server.send(400, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"Identificador não informado\"}");
        return;
    }

    uint32_t campaignId = (uint32_t)server.arg("id").toInt();
    if (campaignId == 0 || !storage.exists(campaignId))
    {
        server.send(404, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"Ensaio não encontrado\"}");
        return;
    }

    String body = server.arg("plain");
    String description = extractJsonString(body, "description");
    description.trim();
    if (description.length() > 50)
        description.remove(50);

    if (!storage.updateCampaignDescription(campaignId, description.c_str()))
    {
        server.send(500, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"Falha ao atualizar a descrição\"}");
        return;
    }

    String json = "{\"ok\":true,\"message\":\"Descrição atualizada\",\"description\":\"";
    appendJsonEscaped(json, description.c_str());
    json += "\"}";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json; charset=utf-8", json);
}

void RocketWebServer::handleDeleteCampaign()
{
    if (state != STATE_IDLE)
    {
        server.send(409, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"A exclusão só pode ser feita em IDLE\"}");
        return;
    }

    if (!server.hasArg("id"))
    {
        server.send(400, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"Identificador não informado\"}");
        return;
    }

    uint32_t campaignId = (uint32_t)server.arg("id").toInt();
    if (campaignId == 0 || !storage.exists(campaignId))
    {
        server.send(404, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"Ensaio não encontrado\"}");
        return;
    }

    if (!storage.deleteCampaign(campaignId))
    {
        server.send(500, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"Falha ao apagar o ensaio\"}");
        return;
    }

    String json = "{\"ok\":true,\"message\":\"Ensaio apagado\",\"freeBytes\":";
    json += String((unsigned long)storage.getFreeBytes());
    json += "}";
    server.send(200, "application/json; charset=utf-8", json);
}

void RocketWebServer::handleForceSign()
{
    if (state != STATE_IDLE)
    {
        server.send(409, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"A inversão de sinal só pode ser alterada em IDLE\"}");
        return;
    }

    bool inverted = false;
    if (!extractJsonBool(server.arg("plain"), "inverted", inverted))
    {
        server.send(400, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"Configuração inválida\"}");
        return;
    }

    if (!Scale.setInverted(inverted))
    {
        server.send(500, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"Não foi possível salvar a configuração\"}");
        return;
    }

    String json = "{\"ok\":true,\"message\":\"Sinal da força atualizado\",\"forceInverted\":";
    json += Scale.isInverted() ? "true" : "false";
    json += "}";
    server.send(200, "application/json; charset=utf-8", json);
}


void RocketWebServer::handleTriggerSetting()
{
    if (state != STATE_IDLE)
    {
        server.send(409, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"O trigger só pode ser alterado em IDLE\"}");
        return;
    }

    float value = 0.0f;
    if (!extractJsonFloat(server.arg("plain"), "triggerGrams", value) ||
        value < MIN_TRIGGER_GRAMS || value > MAX_TRIGGER_GRAMS)
    {
        server.send(400, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"Trigger inválido\"}");
        return;
    }

    if (!acquisitionSetTriggerGrams(value))
    {
        server.send(500, "application/json; charset=utf-8",
                    "{\"ok\":false,\"message\":\"Não foi possível salvar o trigger\"}");
        return;
    }

    String json = "{\"ok\":true,\"message\":\"Trigger atualizado\",\"triggerGrams\":";
    json += String(acquisitionGetTriggerGrams(), 1);
    json += ",\"stopGrams\":";
    json += String(acquisitionGetStopGrams(), 1);
    json += "}";
    server.send(200, "application/json; charset=utf-8", json);
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
        case STATE_POST_RECORDING: return "POST_RECORDING";
        case STATE_PROCESSING: return "PROCESSING";
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
    json += ",\"forceInverted\":";
    json += Scale.isInverted() ? "true" : "false";
    json += ",\"triggerGrams\":";
    json += String(acquisitionGetTriggerGrams(), 1);
    json += ",\"stopGrams\":";
    json += String(acquisitionGetStopGrams(), 1);
    json += ",\"lastCampaign\":";
    json += String(storage.getLastCampaignId());
    json += ",\"campaignCount\":";
    json += String(storage.getCampaignCount());
    json += ",\"storageTotal\":";
    json += String((unsigned long)storage.getTotalBytes());
    json += ",\"storageUsed\":";
    json += String((unsigned long)storage.getUsedBytes());
    json += ",\"storageFree\":";
    json += String((unsigned long)storage.getFreeBytes());
    json += "}";
    return json;
}
