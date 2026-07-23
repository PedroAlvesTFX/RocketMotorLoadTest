Rocket Motor Load Test
-------------
É um sistema para testar motores de foguete de baixa potência.
Composto de:

- ESP32S3
- HX711
- célula de carga

<img width="596" height="593" alt="TestStand" src="https://github.com/user-attachments/assets/8154f017-83eb-47ac-8dba-381938420270" />

<img width="1280" height="720" alt="hardware" src="https://github.com/user-attachments/assets/78489a77-414a-47de-beb8-721e261d7bd7" />

<img width="1200" height="675" alt="hx711-detail-80SPS" src="https://github.com/user-attachments/assets/2811c00e-7a3e-4860-a3eb-1380497ad78d" />

<img width="434" height="502" alt="Ignition" src="https://github.com/user-attachments/assets/a6d3b3ac-c89a-4298-bb5b-5f184aa2c089" />

**Configuração:**
<sub>
 ```// ==============================
 // Hardware
 // ============================== 
 #define HX711_DOUT_PIN      13
 #define HX711_SCK_PIN       12
 // ==============================
 // WiFi Access Point
 // ==============================
 #define AP_SSID             "RocketTestStand"
 #define AP_PASSWORD         "rocket123"
 // ==============================
 // Acquisition
 // ==============================
 #define SAMPLE_RATE         80
```
</sub>

