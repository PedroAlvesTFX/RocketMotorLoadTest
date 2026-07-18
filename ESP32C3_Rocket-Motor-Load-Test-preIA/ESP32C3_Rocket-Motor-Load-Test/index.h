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

body{
    background:#111;
    color:#EEE;
    font-family:Arial,Helvetica,sans-serif;
    text-align:center;
    margin:0;
    padding:20px;
}

h1{
    margin-bottom:5px;
}

.subtitle{
    color:#AAA;
    margin-bottom:30px;
}

.card{
    display:inline-block;
    width:280px;
    background:#222;
    border-radius:12px;
    padding:20px;
    margin:10px;
    box-shadow:0 0 12px #000;
}

.label{
    color:#AAA;
    font-size:15px;
}

.value{
    font-size:48px;
    font-weight:bold;
    margin-top:10px;
}

.state{
    font-size:24px;
    color:#00FF80;
}

button{

    width:220px;

    height:55px;

    margin:10px;

    font-size:20px;

    border:none;

    border-radius:8px;

    cursor:pointer;

}

.start{
    background:#009933;
    color:white;
}

.stop{
    background:#BB0000;
    color:white;
}

.tare{
    background:#0066CC;
    color:white;
}

.footer{

    margin-top:40px;

    color:#888;

    font-size:13px;

}

</style>

</head>

<body>

<h1>🚀 Rocket Motor Test Stand</h1>

<div class="subtitle">
ESP32-C3 + HX711
</div>

<div class="card">

<div class="label">
STATUS
</div>

<div id="status" class="state">
IDLE
</div>

</div>

<div class="card">

<div class="label">
FORÇA
</div>

<div id="force" class="value">
0.0
</div>

<div>
gramas
</div>

</div>

<div class="card">

<div class="label">
PICO
</div>

<div id="peak" class="value">
0.0
</div>

<div>
gramas
</div>

</div>

<br>

<button class="tare" onclick="tare()">
Tara
</button>

<button class="start" onclick="startTest()">
Iniciar Ensaio
</button>

<button onclick="tare()">
Tara
</button>

<button onclick="stopTest()">
Parar
</button>

<button class="stop" onclick="stopTest()">
Parar
</button>

<div class="footer">

Rocket Motor Static Test Stand

</div>

<script>

function refresh()
{
    fetch("/status")
    .then(r=>r.json())
    .then(j=>{

        document.getElementById("status").innerHTML=j.state;

        document.getElementById("force").innerHTML=
            Number(j.force).toFixed(1);

        document.getElementById("peak").innerHTML=
            Number(j.peak).toFixed(1);

    });
}

setInterval(refresh,100);

refresh();

function startTest()
{

    let d=new Date();

    fetch("/start",{

        method:"POST",

        headers:{

            "Content-Type":"application/json"

        },

        body:JSON.stringify({

            datetime:d.toISOString()

        })

    });

}

function tare()
{
    fetch("/tare",{method:"POST"});
}

function stopTest()
{
    fetch("/stop",{method:"POST"});
}

function tare()
{
    fetch("/tare",
    {
        method:"POST"
    });
}

function stopTest()
{
    fetch("/stop",
    {
        method:"POST"
    });
}

</script>

</body>

</html>
)rawliteral";

#endif