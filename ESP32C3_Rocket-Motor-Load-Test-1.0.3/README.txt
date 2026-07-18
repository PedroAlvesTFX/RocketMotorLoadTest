Rocket Motor Test Stand - Release 1.0

Placa: ESP32 com Arduino Framework
Sensor: HX711 usando a biblioteca HX711_ADC

Bibliotecas necessárias:
- ESP32 Arduino Core
- HX711_ADC

Configuração:
- Pinos, calibração e limites estão em config.h
- SSID padrão: RocketTestStand
- Senha padrão: rocket123
- Endereço Web: http://192.168.4.1

Operação:
1. Energize o equipamento sem carga.
2. Conecte-se ao Access Point.
3. Abra http://192.168.4.1.
4. Execute Tara.
5. Clique em Iniciar Ensaio.
6. O sistema aguarda força >= TRIGGER_GRAMS por três leituras.
7. Os dados são gravados até a força permanecer abaixo de STOP_GRAMS
   durante STOP_TIME_MS ou até MAX_TEST_TIME_MS.
8. Use Baixar último CSV para obter a campanha mais recente.

Observação:
Confirme HX711_DOUT_PIN e HX711_SCK_PIN em config.h para sua placa física.

Release 1.0.2: correção do protocolo HTTP no download CSV, Content-Length exato e envio em buffer.
