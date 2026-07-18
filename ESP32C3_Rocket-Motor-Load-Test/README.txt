Rocket Motor Load Test - Release 1.2.3

Principais alterações:
- Camada de processamento consolidada no firmware.
- Estatísticas calculadas uma única vez ao salvar o ensaio.
- Estatísticas gravadas no cabeçalho do arquivo RTS.
- Página Web apenas exibe os resultados calculados pelo ESP32.
- Correção dos campos NaN na página de análise.
- Armazenamento de índices de ignição, pico e fim da queima.
- Cálculo de pico, mínimo, média, impulso trapezoidal, duração,
  ignição, instante do pico, tempo até o pico, tempo de queima,
  média durante a queima, taxa real e taxa máxima de subida.
- CSV passa a incluir as estatísticas no cabeçalho.
- Importação CSV recalcula e grava todas as estatísticas.
- Comparação de curvas, marcadores, área de impulso, backup RTS,
  restauração, edição de descrição e gestão automática de espaço mantidos.

ATENÇÃO:
O formato de armazenamento foi atualizado para a versão 3.
Na primeira inicialização, campanhas antigas existentes no LittleFS serão apagadas.
Exporte antes em CSV qualquer ensaio que deseje preservar. CSVs exportados podem
ser importados novamente nesta versão e terão suas estatísticas recalculadas.

Release 1.2.3:
- Configuração Web persistente para inverter o sinal da força.
- A opção é aplicada antes do trigger, gravação e análise.
- A configuração é armazenada em Preferences/NVS e preservada após reinicialização.


Release 1.2.3:
- Trigger configurável pela interface Web (0,5 g a 10000 g).
- Valor salvo em Preferences/NVS.
- Alteração permitida somente em IDLE.
- Para triggers de 10 g ou menos, o limite de término é ajustado automaticamente para 50% do trigger.


Release 1.2.4
---------------
- Mantém a aquisição por 2 segundos após a confirmação do fim da queima.
- O fim continua sendo confirmado pela força abaixo do limite por STOP_TIME_MS.
- A janela pós-trigger registra a linha de base final e centraliza melhor a curva.
- O buffer foi ampliado para 2900 amostras.
- O formato dos arquivos RTS permanece compatível com a Release 1.2.3.

Release 2.0.0
-------------
- Núcleo de aquisição reestruturado.
- Captura começa imediatamente em ARMED.
- HTTP/Wi-Fi suspensos durante todo o ciclo ativo, inclusive ARMED.
- Pré-trigger circular limitado por tempo real: exatamente até 2 s antes do trigger.
- A duração do pré-trigger não depende de SAMPLE_RATE nem da taxa efetiva do HX711.
- Estados separados para RECORDING, POST_RECORDING, PROCESSING e SAVING.
- Pós-trigger de 2 s mantido após confirmação do fim da queima.
- Taxa de aquisição calculada somente com as amostras efetivamente gravadas.
- Linha temporal da campanha inicia na primeira amostra válida do pré-trigger.
