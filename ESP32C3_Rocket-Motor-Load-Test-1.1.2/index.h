#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="pt-BR"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Rocket Motor Test Stand</title><style>
body{background:#111;color:#eee;font-family:Arial,Helvetica,sans-serif;text-align:center;margin:0;padding:20px}
h1{margin-bottom:5px}
.subtitle{color:#aaa;margin-bottom:24px}
.card{display:inline-block;width:280px;background:#222;border-radius:12px;padding:20px;margin:10px;box-shadow:0 0 12px #000}
.label{color:#aaa;font-size:15px}
.value{font-size:48px;font-weight:bold;margin-top:10px}
.state{font-size:24px;color:#00ff80}
button{width:220px;height:55px;margin:10px;font-size:19px;border:0;border-radius:8px;cursor:pointer}
.start{background:#093;color:#fff}
.stop{background:#b00000;color:#fff}
.tare{background:#06c;color:#fff}
.history-button{background:#555;color:#fff}
#message{min-height:24px;margin:12px 0;color:#ffd166}
.description-wrap{max-width:960px;margin:12px auto 8px;text-align:left}
.description-wrap label{display:block;color:#aaa;font-size:14px;margin:0 0 7px 4px}
.description-line{display:flex;align-items:center;gap:10px}
#testDescription{flex:1;box-sizing:border-box;height:46px;background:#222;color:#fff;border:1px solid #444;border-radius:8px;padding:0 14px;font-size:17px;outline:none}
#testDescription:focus{border-color:#00a86b;box-shadow:0 0 0 2px rgba(0,168,107,.18)}
#descriptionCount{min-width:48px;color:#999;text-align:right;font-size:13px}
.history{max-width:1000px;margin:30px auto 0;text-align:left;background:#1b1b1b;border-radius:12px;padding:18px;box-shadow:0 0 12px #000}
.history h2{text-align:center;margin-top:0}
.table-wrap{width:100%;overflow-x:auto;-webkit-overflow-scrolling:touch}
table{width:100%;border-collapse:collapse;table-layout:fixed;min-width:760px}
th,td{padding:12px 10px;border-bottom:1px solid #383838;vertical-align:middle}
th{color:#aaa;font-weight:bold}
th:nth-child(1),td:nth-child(1){width:75px;text-align:left}
th:nth-child(2),td:nth-child(2){width:235px;text-align:left;white-space:nowrap}
th:nth-child(3),td:nth-child(3){width:260px;text-align:left}
th:nth-child(4),td:nth-child(4){width:100px;text-align:right}
th:nth-child(5),td:nth-child(5){width:90px;text-align:right}
th:nth-child(6),td:nth-child(6){width:145px;text-align:center;white-space:nowrap}
td:nth-child(3){overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
a.action,button.action{display:inline-flex;align-items:center;justify-content:center;width:38px;height:34px;padding:0;margin:0 3px;border:0;border-radius:6px;background:#444;color:#fff;text-decoration:none;cursor:pointer;font-size:16px;line-height:1;vertical-align:middle}
a.action:hover,button.action:hover{filter:brightness(1.18)}
button.action.delete{width:34px;background:#a51f1f!important}
button.action.delete:hover{background:#d12626!important}
.storage{max-width:1000px;margin:18px auto;background:#222;border-radius:10px;padding:14px}
.storage-low{color:#ffd166}
.storage-critical{color:#ff8080}
.empty{text-align:center!important;color:#999;padding:22px}
.footer{margin-top:35px;color:#888;font-size:13px}
@media(max-width:700px){body{padding:10px}.history{padding:10px}table{min-width:720px}th,td{padding:10px 8px}}
</style></head><body>
<h1>🚀 Rocket Motor Test Stand</h1><div class="subtitle">ESP32 + HX711</div>
<div class="card"><div class="label">STATUS</div><div id="status" class="state">IDLE</div></div>
<div class="card"><div class="label">FORÇA</div><div id="force" class="value">0.0</div><div>gramas</div></div>
<div class="card"><div class="label">PICO</div><div id="peak" class="value">0.0</div><div>gramas</div></div><br>
<div class="description-wrap"><label for="testDescription">Descrição do ensaio</label><div class="description-line"><input id="testDescription" type="text" maxlength="50" placeholder="Ex.: KNSU 65/35, bocal 6 mm, lote 03" oninput="updateDescriptionCount()"><span id="descriptionCount">0/50</span></div></div>
<button class="tare" onclick="tareScale()">Tara</button><button class="start" onclick="startTest()">Iniciar Ensaio</button><button class="stop" onclick="stopTest()">Parar</button><button class="history-button" onclick="loadHistory()">Atualizar histórico</button>
<div id="message"></div><div class="storage"><strong>Armazenamento:</strong> <span id="storageText">Carregando...</span></div><section class="history"><h2>Histórico de ensaios</h2><div class="table-wrap"><table><thead><tr><th>Ensaio</th><th>Data e hora</th><th>Descrição</th><th>Pico</th><th>Amostras</th><th>Ações</th></tr></thead><tbody id="historyBody"><tr><td colspan="6" class="empty">Carregando...</td></tr></tbody></table></div></section><div class="footer">Rocket Motor Static Test Stand</div>
<script>
let previousState='',refreshBusy=false,communicationFailures=0;function message(t){document.getElementById('message').textContent=t}function updateDescriptionCount(){const e=document.getElementById('testDescription'),c=document.getElementById('descriptionCount');c.textContent=e.value.length+'/50'}function escapeHtml(v){return String(v||'').replace(/[&<>\"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','\"':'&quot;',"'":'&#39;'}[c]))}function pad(n){return String(n).padStart(2,'0')}function browserDateTime(){const d=new Date(),o=-d.getTimezoneOffset(),s=o>=0?'+':'-',hh=pad(Math.floor(Math.abs(o)/60)),mm=pad(Math.abs(o)%60);return d.getFullYear()+'-'+pad(d.getMonth()+1)+'-'+pad(d.getDate())+'T'+pad(d.getHours())+':'+pad(d.getMinutes())+':'+pad(d.getSeconds())+s+hh+':'+mm}function displayDateTime(v){return !v||v==='unknown'?'Não informada':v.replace('T',' ')}function formatBytes(v){if(v<1024)return v+' B';if(v<1048576)return (v/1024).toFixed(1)+' KB';return (v/1048576).toFixed(2)+' MB'}function updateStorage(j){const e=document.getElementById('storageText'),pct=j.storageTotal?100*j.storageUsed/j.storageTotal:0;e.textContent=formatBytes(j.storageUsed)+' usados de '+formatBytes(j.storageTotal)+' — '+formatBytes(j.storageFree)+' livres ('+pct.toFixed(1)+'%)';e.className=pct>=90?'storage-critical':pct>=80?'storage-low':''}
async function refresh(){if(refreshBusy)return;refreshBusy=true;try{const r=await fetch('/status',{cache:'no-store'});if(!r.ok)throw new Error('HTTP '+r.status);const j=await r.json();communicationFailures=0;if(document.getElementById('message').textContent==='Sem comunicação com o equipamento')message('');document.getElementById('status').textContent=j.state;document.getElementById('force').textContent=Number(j.force).toFixed(1);document.getElementById('peak').textContent=Number(j.peak).toFixed(1);updateStorage(j);if(previousState&&previousState!=='IDLE'&&j.state==='IDLE')loadHistory();previousState=j.state}catch(e){communicationFailures++;if(communicationFailures>=3)message('Sem comunicação com o equipamento')}finally{refreshBusy=false}}
async function startTest(){try{const r=await fetch('/start',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({datetime:browserDateTime(),description:document.getElementById('testDescription').value.trim().slice(0,50)})});message(await r.text())}catch(e){message('Falha ao iniciar o ensaio')}}
async function tareScale(){try{const r=await fetch('/tare',{method:'POST'});message(await r.text())}catch(e){message('Falha ao executar tara')}}async function stopTest(){try{const r=await fetch('/stop',{method:'POST'});message(await r.text())}catch(e){message('Falha ao parar o ensaio')}}
async function loadHistory(){const b=document.getElementById('historyBody');try{const r=await fetch('/history',{cache:'no-store'});if(!r.ok)throw 0;const d=await r.json();if(!d.campaigns.length){b.innerHTML='<tr><td colspan="6" class="empty">Nenhum ensaio gravado</td></tr>';return}b.innerHTML=d.campaigns.map(c=>'<tr><td>#'+c.id+'</td><td>'+displayDateTime(c.datetime)+'</td><td>'+escapeHtml(c.description||'Sem descrição')+'</td><td>'+Number(c.peak).toFixed(1)+' g</td><td>'+c.samples+'</td><td><a class="action" href="/test?id='+c.id+'" title="Visualizar gráfico" aria-label="Visualizar gráfico">📈</a><a class="action" href="/csv?id='+c.id+'" title="Baixar CSV" aria-label="Baixar CSV">📄</a><button class="action delete" title="Apagar ensaio" aria-label="Apagar ensaio" onclick="deleteTest('+c.id+')">🗑</button></td></tr>').join('')}catch(e){b.innerHTML='<tr><td colspan="6" class="empty">Falha ao carregar o histórico</td></tr>'}}
async function deleteTest(id){if(!confirm('Apagar definitivamente o ensaio #'+id+'?'))return;try{const r=await fetch('/campaign/delete?id='+id,{method:'POST'}),j=await r.json();message(j.message||'Operação concluída');if(r.ok){await loadHistory();await refresh()}}catch(e){message('Falha ao apagar o ensaio')}}setInterval(refresh,500);updateDescriptionCount();refresh();loadHistory();
</script></body></html>
)rawliteral";

const char TEST_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="pt-BR"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Detalhes do ensaio</title><style>
body{background:#111;color:#eee;font-family:Arial,Helvetica,sans-serif;margin:0;padding:18px}.wrap{max-width:1100px;margin:auto}a{color:#fff}.top{display:flex;gap:10px;align-items:center;justify-content:space-between;flex-wrap:wrap}.button{background:#444;padding:10px 15px;border-radius:7px;text-decoration:none}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(155px,1fr));gap:12px;margin:20px 0}.card{background:#222;border-radius:10px;padding:16px}.label{color:#aaa;font-size:13px}.value{font-size:25px;font-weight:bold;margin-top:7px}.chartbox{background:#181818;border-radius:12px;padding:12px;box-shadow:0 0 12px #000}canvas{width:100%;height:460px;display:block;touch-action:none}.hint{color:#999;text-align:center;font-size:13px;margin-top:8px}.error{color:#ff8080;padding:30px;text-align:center}
</style></head><body><div class="wrap"><div class="top"><div><h1 id="title">Ensaio</h1><div id="description" style="font-size:18px;margin:5px 0;color:#ddd">Sem descrição</div><div id="datetime">Carregando...</div></div><div><a class="button" href="/">Voltar</a> <a id="csv" class="button" href="#">Baixar CSV</a> <button class="button" style="border:0;color:#fff;background:#8b1a1a;cursor:pointer" onclick="deleteCurrent()">Apagar</button></div></div>
<div class="grid"><div class="card"><div class="label">PICO</div><div id="peak" class="value">-</div></div><div class="card"><div class="label">IMPULSO POSITIVO</div><div id="impulse" class="value">-</div></div><div class="card"><div class="label">DURAÇÃO REGISTRADA</div><div id="duration" class="value">-</div></div><div class="card"><div class="label">FORÇA MÉDIA</div><div id="average" class="value">-</div></div><div class="card"><div class="label">FORÇA MÍNIMA</div><div id="minimum" class="value">-</div></div><div class="card"><div class="label">AMOSTRAS</div><div id="samples" class="value">-</div></div></div>
<div class="chartbox"><canvas id="chart"></canvas><div class="hint">Arraste horizontalmente para ampliar uma região. Toque duas vezes ou clique em “Restaurar” para voltar.</div><div style="text-align:center;margin-top:10px"><button onclick="resetZoom()">Restaurar</button></div></div><div id="error" class="error"></div></div>
<script>
const id=new URLSearchParams(location.search).get('id');let points=[],viewStart=0,viewEnd=0,dragStart=null;const canvas=document.getElementById('chart'),ctx=canvas.getContext('2d');function fmtDate(v){return !v||v==='unknown'?'Não informada':v.replace('T',' ')}function resize(){const d=devicePixelRatio||1,r=canvas.getBoundingClientRect();canvas.width=Math.max(300,Math.floor(r.width*d));canvas.height=Math.max(260,Math.floor(r.height*d));ctx.setTransform(d,0,0,d,0,0);draw()}window.addEventListener('resize',resize);
function draw(){const w=canvas.clientWidth,h=canvas.clientHeight;ctx.clearRect(0,0,w,h);if(points.length<2)return;const a=Math.max(0,viewStart),b=Math.min(points.length-1,viewEnd),slice=points.slice(a,b+1);let ymin=Infinity,ymax=-Infinity;for(const p of slice){if(p[1]<ymin)ymin=p[1];if(p[1]>ymax)ymax=p[1]}if(ymin===ymax){ymin-=1;ymax+=1}const pad={l:62,r:18,t:18,b:42},cw=w-pad.l-pad.r,ch=h-pad.t-pad.b,t0=slice[0][0],t1=slice[slice.length-1][0],x=t=>pad.l+(t-t0)/(t1-t0||1)*cw,y=v=>pad.t+(ymax-v)/(ymax-ymin)*ch;ctx.strokeStyle='#555';ctx.lineWidth=1;ctx.fillStyle='#aaa';ctx.font='12px Arial';for(let i=0;i<=5;i++){const yy=pad.t+i*ch/5,val=ymax-i*(ymax-ymin)/5;ctx.beginPath();ctx.moveTo(pad.l,yy);ctx.lineTo(w-pad.r,yy);ctx.stroke();ctx.fillText(val.toFixed(0)+' g',4,yy+4)}for(let i=0;i<=5;i++){const xx=pad.l+i*cw/5,val=(t0+i*(t1-t0)/5)/1e6;ctx.beginPath();ctx.moveTo(xx,pad.t);ctx.lineTo(xx,h-pad.b);ctx.stroke();ctx.fillText(val.toFixed(1)+' s',xx-12,h-15)}ctx.strokeStyle='#00d084';ctx.lineWidth=2;ctx.beginPath();slice.forEach((p,i)=>{const xx=x(p[0]),yy=y(p[1]);i?ctx.lineTo(xx,yy):ctx.moveTo(xx,yy)});ctx.stroke();ctx.strokeStyle='#888';ctx.beginPath();ctx.moveTo(pad.l,y(0));ctx.lineTo(w-pad.r,y(0));ctx.stroke()}
function indexAt(px){const w=canvas.clientWidth,padL=62,padR=18,r=Math.max(0,Math.min(1,(px-padL)/(w-padL-padR)));return Math.round(viewStart+r*(viewEnd-viewStart))}canvas.addEventListener('pointerdown',e=>{dragStart=e.offsetX;canvas.setPointerCapture(e.pointerId)});canvas.addEventListener('pointerup',e=>{if(dragStart===null)return;const a=indexAt(dragStart),b=indexAt(e.offsetX);dragStart=null;if(Math.abs(b-a)>4){viewStart=Math.min(a,b);viewEnd=Math.max(a,b);draw()}});canvas.addEventListener('dblclick',resetZoom);function resetZoom(){viewStart=0;viewEnd=Math.max(0,points.length-1);draw()}
async function deleteCurrent(){if(!id||!confirm('Apagar definitivamente este ensaio?'))return;try{const r=await fetch('/campaign/delete?id='+id,{method:'POST'}),j=await r.json();if(!r.ok)throw new Error(j.message||'Falha');location.href='/'}catch(e){document.getElementById('error').textContent=e.message||'Falha ao apagar o ensaio'}}async function load(){const errorBox=document.getElementById('error'),csvLink=document.getElementById('csv');if(!id){errorBox.textContent='Identificador do ensaio não informado.';return}csvLink.href='/csv?id='+id;try{const [ir,dr]=await Promise.all([fetch('/campaign?id='+id,{cache:'no-store'}),fetch('/campaign/data?id='+id,{cache:'no-store'})]);if(!ir.ok||!dr.ok)throw 0;const info=await ir.json(),data=await dr.json();points=data.samples||[];document.getElementById('title').textContent='Ensaio #'+info.id;document.getElementById('description').textContent=info.description||'Sem descrição';document.getElementById('datetime').textContent=fmtDate(info.datetime);document.getElementById('peak').textContent=Number(info.peak).toFixed(1)+' g';document.getElementById('impulse').textContent=Number(info.impulse).toFixed(1)+' g·s';document.getElementById('duration').textContent=Number(info.duration).toFixed(3)+' s';document.getElementById('average').textContent=Number(info.average).toFixed(1)+' g';document.getElementById('minimum').textContent=Number(info.minimum).toFixed(1)+' g';document.getElementById('samples').textContent=info.samples;resetZoom();resize()}catch(e){errorBox.textContent='Não foi possível carregar este ensaio.'}}load();
</script></body></html>
)rawliteral";

#endif
