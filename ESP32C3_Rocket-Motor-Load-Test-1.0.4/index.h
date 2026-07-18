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
.start{background:#093;color:#fff}.stop{background:#b00000;color:#fff}.tare{background:#06c;color:#fff}.history-button{background:#555;color:#fff}
#message{min-height:24px;margin:12px 0;color:#ffd166}
.history{max-width:900px;margin:30px auto 0;text-align:left;background:#1b1b1b;border-radius:12px;padding:18px;box-shadow:0 0 12px #000}
.history h2{text-align:center;margin-top:0}.table-wrap{overflow-x:auto}
table{width:100%;border-collapse:collapse;min-width:650px}th,td{padding:11px 9px;border-bottom:1px solid #383838;text-align:left}th{color:#aaa}td.num{text-align:right}
a.download{display:inline-block;background:#444;color:white;text-decoration:none;padding:7px 12px;border-radius:6px}.empty{text-align:center;color:#999;padding:22px}
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
<button class="history-button" onclick="loadHistory()">Atualizar histórico</button>
<div id="message"></div>
<section class="history">
<h2>Histórico de ensaios</h2>
<div class="table-wrap">
<table>
<thead><tr><th>Ensaio</th><th>Data e hora</th><th>Pico</th><th>Amostras</th><th>Arquivo</th></tr></thead>
<tbody id="historyBody"><tr><td colspan="5" class="empty">Carregando...</td></tr></tbody>
</table>
</div>
</section>
<div class="footer">Rocket Motor Static Test Stand</div>
<script>
let lastCampaign=0;
let previousState='';
function message(text){document.getElementById('message').textContent=text}
function pad(n){return String(n).padStart(2,'0')}
function browserDateTime(){
 const d=new Date();
 const offset=-d.getTimezoneOffset();
 const sign=offset>=0?'+':'-';
 const hh=pad(Math.floor(Math.abs(offset)/60));
 const mm=pad(Math.abs(offset)%60);
 return d.getFullYear()+'-'+pad(d.getMonth()+1)+'-'+pad(d.getDate())+'T'+pad(d.getHours())+':'+pad(d.getMinutes())+':'+pad(d.getSeconds())+sign+hh+':'+mm;
}
function displayDateTime(value){
 if(!value||value==='unknown') return 'Não informada';
 return value.replace('T',' ');
}
async function refresh(){
 try{
  const r=await fetch('/status',{cache:'no-store'}); const j=await r.json();
  document.getElementById('status').textContent=j.state;
  document.getElementById('force').textContent=Number(j.force).toFixed(1);
  document.getElementById('peak').textContent=Number(j.peak).toFixed(1);
  lastCampaign=Number(j.lastCampaign||0);
  if(previousState && previousState!=='IDLE' && j.state==='IDLE') loadHistory();
  previousState=j.state;
 }catch(e){message('Sem comunicação com o equipamento')}
}
async function startTest(){
 try{
  const r=await fetch('/start',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({datetime:browserDateTime()})});
  message(await r.text());
 }catch(e){message('Falha ao iniciar o ensaio')}
}
async function tareScale(){try{const r=await fetch('/tare',{method:'POST'});message(await r.text())}catch(e){message('Falha ao executar tara')}}
async function stopTest(){try{const r=await fetch('/stop',{method:'POST'});message(await r.text())}catch(e){message('Falha ao parar o ensaio')}}
async function loadHistory(){
 const body=document.getElementById('historyBody');
 try{
  const r=await fetch('/history',{cache:'no-store'});
  if(!r.ok) throw new Error();
  const data=await r.json();
  if(!data.campaigns.length){body.innerHTML='<tr><td colspan="5" class="empty">Nenhum ensaio gravado</td></tr>';return}
  body.innerHTML=data.campaigns.map(c=>'<tr><td>#'+c.id+'</td><td>'+displayDateTime(c.datetime)+'</td><td class="num">'+Number(c.peak).toFixed(1)+' g</td><td class="num">'+c.samples+'</td><td><a class="download" href="/csv?id='+c.id+'">CSV</a></td></tr>').join('');
 }catch(e){body.innerHTML='<tr><td colspan="5" class="empty">Falha ao carregar o histórico</td></tr>'}
}
setInterval(refresh,250); refresh(); loadHistory();
</script>
</body>
</html>
)rawliteral";

#endif
