#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Rocket Motor Test Stand</title>
<style>
body{background:#111;color:#eee;font-family:Arial,Helvetica,sans-serif;text-align:center;margin:0;padding:20px}
h1{margin-bottom:5px}.subtitle{color:#aaa;margin-bottom:24px}
.card{display:inline-block;width:280px;background:#222;border-radius:12px;padding:20px;margin:10px;box-shadow:0 0 12px #000}
.label{color:#aaa;font-size:15px}.value{font-size:48px;font-weight:bold;margin-top:10px}.state{font-size:24px;color:#00ff80}
button{width:220px;height:55px;margin:10px;font-size:19px;border:0;border-radius:8px;cursor:pointer}
.start{background:#093;color:#fff}.stop{background:#b00000;color:#fff}.tare{background:#06c;color:#fff}.csv{background:#555;color:#fff}
.footer{margin-top:35px;color:#888;font-size:13px}
</style>
</head>
<body>
<h1>🚀 Rocket Motor Test Stand</h1>
<div class="subtitle">ESP32 + HX711</div>
<div class="card"><div class="label">STATUS</div><div id="status" class="state">IDLE</div></div>
<div class="card"><div class="label">FORÇA</div><div id="force" class="value">0.0</div><div>gramas</div></div>
<div class="card"><div class="label">PICO</div><div id="peak" class="value">0.0</div><div>gramas</div></div>
<br>
<button class="tare" onclick="tareScale()">Tara</button>
<button class="start" onclick="startTest()">Iniciar Ensaio</button>
<button class="stop" onclick="stopTest()">Parar</button>
<button class="csv" onclick="downloadCSV()">Baixar último CSV</button>
<div id="message"></div>
<div class="footer">Rocket Motor Static Test Stand</div>
<script>
let lastCampaign=0;
function message(text){document.getElementById('message').textContent=text}
async function refresh(){
 try{
  const r=await fetch('/status',{cache:'no-store'}); const j=await r.json();
  document.getElementById('status').textContent=j.state;
  document.getElementById('force').textContent=Number(j.force).toFixed(1);
  document.getElementById('peak').textContent=Number(j.peak).toFixed(1);
  lastCampaign=Number(j.lastCampaign||0);
 }catch(e){message('Sem comunicação com o equipamento')}
}
async function startTest(){
 const r=await fetch('/start',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({datetime:new Date().toISOString()})});
 message(await r.text());
}
async function tareScale(){const r=await fetch('/tare',{method:'POST'});message(await r.text())}
async function stopTest(){const r=await fetch('/stop',{method:'POST'});message(await r.text())}
function downloadCSV(){if(lastCampaign>0) location.href='/csv?id='+lastCampaign; else message('Nenhuma campanha gravada')}
setInterval(refresh,250); refresh();
</script>
</body>
</html>
)rawliteral";

#endif
