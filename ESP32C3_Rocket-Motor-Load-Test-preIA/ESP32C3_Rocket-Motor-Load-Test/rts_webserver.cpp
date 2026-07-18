/******************************************************************************
 *
 * Rocket Test Stand
 *
 * webserver.cpp
 *
 ******************************************************************************/

#include "rts_webserver.h"

#include "globals.h"
#include "campaign.h"
#include "rts_storage.h"
#include "rts_acquisition.h"

RocketWebServer webServer;

/******************************************************************************
 * HTML principal
 ******************************************************************************/

static const char MAIN_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Rocket Motor Test Stand</title>
<style>
body{
    font-family:Arial;
    margin:40px;
    background:#202124;
    color:white;
}

button{
    width:220px;
    height:60px;
    font-size:20px;
    margin:10px;
}

.card{
    background:#303134;
    padding:20px;
    border-radius:10px;
    margin-bottom:20px;
}

.value{
    font-size:32px;
    color:#00ff80;
}

</style>
</head>
<body>
<h1>Rocket Motor Test Stand</h1>
<div class="card">
Estado:
<div id="state" class="value">
IDLE
</div>
</div>
<div class="card">
Leitura
<div id="force" class="value">
0.0 g
</div>
</div>
<button onclick="startTest()">
Iniciar Ensaio
</button>
<button onclick="tare()">
Tara
</button>
<button onclick="stopTest()">
Parar
</button>
<button onclick="location.reload()">
Atualizar
</button>
<script>
function updateStatus()
{
fetch("/status")
.then(r=>r.json())
.then(j=>{
document.getElementById("state").innerHTML=j.state;
document.getElementById("force").innerHTML=j.force.toFixed(1)+" g";
});
}
function startTest()
{
fetch("/start",
{
method:"POST"
});
}
setInterval(updateStatus,300);
updateStatus();

function tare()
{
    fetch("/tare",{method:"POST"});
}

function stopTest()
{
    fetch("/stop",{method:"POST"});
}

</script>
</body>
</html>
)=====";

/******************************************************************************
 * Construtor
 ******************************************************************************/

RocketWebServer::RocketWebServer() :
server(80)
{

}

/******************************************************************************
 * Inicialização
 ******************************************************************************/

bool RocketWebServer::begin()
{
    setupRoutes();
    server.begin();
    Serial.println();
    Serial.println("HTTP Server Started");
    return true;
}

/******************************************************************************
 * Loop
 ******************************************************************************/

void RocketWebServer::loop()
{
    server.handleClient();
}

/******************************************************************************
 * Rotas
 ******************************************************************************/

void RocketWebServer::setupRoutes()
{

    server.on("/",
              HTTP_GET,
              [this]()
              {
                  handleRoot();
              });

    server.on("/status",
              HTTP_GET,
              [this]()
              {
                  handleStatus();
              });

    server.on("/start",
              HTTP_POST,
              [this]()
              {
                  handleStart();
              });

server.on("/tare",
          HTTP_POST,
          [this]()
          {
              handleTare();
          });

server.on("/stop",
          HTTP_POST,
          [this]()
          {
              handleStop();
          });

server.on(
    "/csv",
    HTTP_GET,
    [this]()
    {
        handleCSV();
    });

    server.onNotFound(
              [this]()
              {
                  handleNotFound();
              });

}

/******************************************************************************
 * Página principal
 ******************************************************************************/
/******************************************************************************
 * Download CSV
 ******************************************************************************/

void RocketWebServer::handleCSV()
{
    server.send(
        501,
        "text/plain",
        "CSV export not implemented");
}

void RocketWebServer::handleRoot()
{
    server.send(
        200,
        "text/html",
        MAIN_PAGE);
}

/******************************************************************************
 * Status
 ******************************************************************************/

void RocketWebServer::handleStatus()
{

    String json;

    json="{";

    json+="\"state\":\"";

    switch(state)
    {
        case STATE_IDLE:
            json+="IDLE";
            break;

        case STATE_ARMED:
            json+="ARMED";
            break;

        case STATE_WAIT_TRIGGER:
            json+="WAIT_TRIGGER";
            break;

        case STATE_RECORDING:
            json+="RECORDING";
            break;

        case STATE_SAVING:
            json+="SAVING";
            break;

        case STATE_FINISHED:
            json+="FINISHED";
            break;
    }

    json+="\",";

    // Na próxima etapa será substituído pela leitura real do HX711
json += "\"force\":";
json += String(acquisitionGetForce(),1);

json += ",";

json += "\"peak\":";
json += String(acquisitionGetPeak(),1);

json += ",";

json += "\"recording\":";
json += acquisitionIsRecording() ? "true" : "false";

    json+="}";

    server.send(
        200,
        "application/json",
        json);
}

/******************************************************************************
 * Tara
 ******************************************************************************/
void RocketWebServer::handleTare()
{
    acquisitionTare();

    server.send(
        200,
        "text/plain",
        "OK");
}
/******************************************************************************
 * Cancelar ensaio
 ******************************************************************************/
void RocketWebServer::handleStop()
{
    acquisitionStop();

    server.send(
        200,
        "text/plain",
        "OK");
}
/******************************************************************************
 * Iniciar campanha
 ******************************************************************************/

void RocketWebServer::handleStart()
{

    if(state==STATE_IDLE)
    {
        state=STATE_ARMED;
        Serial.println();
        Serial.println("Campaign armed from WEB");
    }

    server.send(
        200,
        "text/plain",
        "OK");
}

/******************************************************************************
 * Página inexistente
 ******************************************************************************/

void RocketWebServer::handleNotFound()
{

    server.send(
        404,
        "text/plain",
        "404");

}