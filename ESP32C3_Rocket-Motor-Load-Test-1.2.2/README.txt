Rocket Motor Load Test - Release 1.2.2

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

Release 1.2.2:
- Configuração Web persistente para inverter o sinal da força.
- A opção é aplicada antes do trigger, gravação e análise.
- A configuração é armazenada em Preferences/NVS e preservada após reinicialização.
