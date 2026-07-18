Rocket Motor Test Stand - Release 1.1.0

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

Release 1.0.4:
- Histórico de ensaios na página principal.
- Metadados de cada campanha: ID, data/hora, pico e amostras.
- Download CSV individual pelo histórico.
- Data/hora local e fuso enviados pelo navegador ao armar o ensaio.

Release 1.1.0:
- Página individual para cada ensaio.
- Gráfico interativo local em Canvas, sem dependência de Internet.
- Zoom horizontal por arraste e restauração por duplo clique.
- Estatísticas calculadas: pico, mínimo, média, duração e impulso positivo.
- Endpoints JSON para metadados e curva do ensaio.
- Compatibilidade preservada com campanhas gravadas pelas versões anteriores.

Release 1.1.1
- Exclusão manual de ensaios pelo histórico e pela página de detalhes.
- Indicador de espaço total, usado e livre do LittleFS.
- Reserva mínima de 32 KB configurada por STORAGE_MIN_FREE_BYTES.
- Antes de salvar, se faltar espaço, o ensaio mais antigo é apagado automaticamente.
- A rotação automática preserva os ensaios mais recentes e repete a exclusão apenas até haver espaço suficiente.

Release 1.1.2
-------------
- Campo de descrição do ensaio com até 50 caracteres.
- Data/hora e descrição enviadas pelo navegador ao armar o ensaio.
- Descrição gravada no cabeçalho binário da campanha.
- Descrição exibida no histórico e na página detalhada do ensaio.
- Metadados do ensaio incluídos no início do CSV.
- Novo formato de armazenamento (versão 2).
- Ao detectar o formato anterior, o LittleFS apaga automaticamente as campanhas antigas e reinicia a numeração.
