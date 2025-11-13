# Sistema de Monitoramento de Água com Outliers em Display TFT  
(ESP32 + SD Card + ILI9341 + Keypad)

Este repositório contém uma **versão estendida** do sistema de monitoramento de água da comunidade rural do Riacho (Amarantina), agora com:

- **interface gráfica em display ILI9341 (TFT 2.4")**
- **entrada de dados via teclado matricial (wokwi-membrane-keypad)**
- **cartão SD em SPI para leitura das medições**
- **botão liga/desliga** que controla quando o usuário pode interagir com a tela e o keypad

É uma evolução do projeto original (ESP-IDF + monitor serial), agora reimplementado em **Arduino core para ESP32**, usando bibliotecas amplamente disponíveis (Adafruit_GFX, Adafruit_ILI9341, SD, Keypad).

Ele continua resolvendo o mesmo problema:

> As medições diárias eram registradas, mas **analisar manualmente** todas as linhas em busca de irregularidades era cansativo e sujeito a erro.  
> Aqui, o ESP32 aplica **regressão linear + análise de resíduos** para detectar **outliers** (medições anômalas) e exibe os alertas diretamente no **display**, com navegação feita pelo **keypad**.

#Projeto de Extensão 1
[Vídeo do projeto de extensão 1 no YouTube](https://youtu.be/-hB9_I5IgXk)
[Repositório do projeto de extensão 1 no GitHub](https://github.com/silvagal/sistema_medicao_fluxo_agua)

#Projeto de Extensão 2
[Vídeo do projeto de extensão 2 no YouTube](https://youtu.be/W7Dq3uiTRVw)
[Repositório do projeto de extensão 2 no GitHub](https://github.com/silvagal/projeto_extensao2_ead.git)

---

## 1. Visão geral do funcionamento

Em resumo, o sistema faz:

1.1. Lê o arquivo `medicoes.txt` gravado no **cartão SD**, contendo:
   - data (`DD/MM/AAAA`)  
   - quantidade de litros (`000123.456789L`)  
   - tempo de funcionamento (`HH:MM:SS`)

1.2. Converte o tempo para **duração em minutos**.

1.3. Ajusta uma **reta (regressão linear)** que relaciona:
   - eixo X: duração em minutos  
   - eixo Y: litros bombeados  

1.4. Para cada medição, calcula o **resíduo**:
   - `residuo = litros_medidos - litros_previstos_pela_reta`

1.5. Calcula o **valor absoluto** do resíduo para medir “quão longe” o ponto está da reta.

1.6. Encontra o **percentil 90** desses valores absolutos:
   - medições com desvio **abaixo ou igual** a esse limiar → **normais**  
   - medições com desvio **acima** desse limiar → marcadas como **outliers**

1.7. O ESP32 apresenta um **menu na tela TFT**, e o usuário escolhe a opção usando o **keypad**:

   - `1` – ver outliers de todo o período  
   - `2` – ver outliers de um ano específico  
   - `3` – ver outliers de um determinado mês/ano  
   - `4` – sair (desliga o modo de interação e volta para “Sistema OFF”)

1.8. Os outliers são exibidos na tela em **páginas de 3 registros**. Quando há mais de 3, o sistema:

   - mostra 3 outliers  
   - pede para o usuário **pressionar qualquer tecla** para ver os próximos 3  
   - repete até mostrar todos

1.9. Um **botão de liga/desliga** alterna entre:
   - **Sistema OFF** (tela parada, aguardando botão)  
   - **Sistema ON** (menu ativo, interação com keypad habilitada)

---

## 2. Estrutura sugerida do repositório

```text
├── src/
│   └── outliers_tft_keypad.ino   # Código Arduino (ESP32 + TFT + SD + Keypad)
|   └── diagram.json              # Código em json com a configuração do esquemático
|   └── Free_Fonts.c              # Código da biblioteca de FOnts
|   └── libraries.txt             # Arquivo com todas as bibliotecas utilizadas no projeto feito no Wokwi
|   └── wokwi-project.txt         # Arquivo com link do Wokwi
├── dados/
│   └── medicoes.txt              # Arquivo de medições usado pelo sistema
├── imagens/
│   ├── esquematico.png           # Esquemático do circuito completo
│   └── tela_menu.png             # Exemplo da tela de menu no ILI9341
└── README.md
````


## 3. Materiais utilizados

3.1. Hardware

   -  ESP32 DevKit v1
   - Display TFT ILI9341 2.4" (SPI)
   - Módulo de cartão SD (SPI)
   - Teclado matricial 4×4 (wokwi-membrane-keypad)
   - Botão push-button (liga/desliga)
   - Cartão SD (real ou simulado no Wokwi)

   Observação: o sensor de vazão não é usado diretamente. As medições de água já vêm prontas em medicoes.txt e são lidas do cartão SD.

 3.2. Software / Bibliotecas
   - Arduino core for ESP32
   - Bibliotecas Arduino:
   - SPI.h
   - SD.h
   - Adafruit_GFX.h
   - Adafruit_ILI9341.h
   - Keypad.h

   [Wokwi](https://wokwi.com/) (opcional) para simulação do circuito:

4. Conexões do circuito
   A seguir, um resumo das ligações principais entre ESP32, SD, TFT, keypad e botão.

4.1. Barramento SPI compartilhado (SD + Display)

O SD e o display ILI9341 compartilham o mesmo barramento SPI (VSPI do ESP32):

```text
ESP32 DevKit v1           Módulo SD (SPI)      Display ILI9341
-----------------         ----------------     ----------------
3V3                  ->   VCC                 VCC
GND                  ->   GND                 GND
GPIO18               ->   SCK                 SCK
GPIO19               ->   DO (MISO)          MISO (se usado)
GPIO23               ->   DI (MOSI)          MOSI
GPIO5                ->   CS                 -
GPIO15               ->   -                  CS
GPIO2                ->   -                  DC (data/command)
RST                  ->   -                  RST (ou 3V3)
3V3                  ->   -                  LED (backlight)
CS do SD: GPIO 5
CS do TFT: GPIO 15
````

```text
Keypad    ESP32
------------    -----
R1          ->  GPIO13
R2          ->  GPIO12
R3          ->  GPIO14
R4          ->  GPIO27
C1          ->  GPIO26
C2          ->  GPIO25
C3          ->  GPIO33
C4          ->  GPIO32
````

5. Botão de liga/desliga
   - A lógica de energia é controlada por uma variável booleana systemOn:
   - KsystemOn = false → “Sistema OFF”, só mostra mensagem pedindo POWER
   - KsystemOn = true → exibe menu e aceita entrada do keypad

6. Trabalhos futuros
   - Sensor de vazão físico: substituir os dados de arquivo por leitura em tempo real.
   - Gráficos na própria tela TFT: desenhar o gráfico tempo × litros com os pontos outliers destacados.
   - Conectividade Wi-Fi: enviar alertas para um servidor web/app sempre que forem detectados outliers.
   - Configuração pelo usuário: permitir alterar o limiar (percentil 90, 95 etc.) pela própria interface do display.
   - Armazenamento de novos registros: permitir que o ESP32 grave novas medições no SD a cada dia, mantendo o histórico atualizado.

Este projeto mostra como técnicas simples de estatística combinadas com um microcontrolador acessível podem apoiar a gestão de recursos hídricos em comunidades rurais, oferecendo uma ferramenta prática para identificar dias “estranhos” no funcionamento do sistema de bombeamento de água.
