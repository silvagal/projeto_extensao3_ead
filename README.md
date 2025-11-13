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

---

## 1. Visão geral do funcionamento

Em resumo, o sistema faz:

1. Lê o arquivo `medicoes.txt` gravado no **cartão SD**, contendo:
   - data (`DD/MM/AAAA`)  
   - quantidade de litros (`000123.456789L`)  
   - tempo de funcionamento (`HH:MM:SS`)

2. Converte o tempo para **duração em minutos**.

3. Ajusta uma **reta (regressão linear)** que relaciona:
   - eixo X: duração em minutos  
   - eixo Y: litros bombeados  

4. Para cada medição, calcula o **resíduo**:
   - `residuo = litros_medidos - litros_previstos_pela_reta`

5. Calcula o **valor absoluto** do resíduo para medir “quão longe” o ponto está da reta.

6. Encontra o **percentil 90** desses valores absolutos:
   - medições com desvio **abaixo ou igual** a esse limiar → **normais**  
   - medições com desvio **acima** desse limiar → marcadas como **outliers**

7. O ESP32 apresenta um **menu na tela TFT**, e o usuário escolhe a opção usando o **keypad**:

   - `1` – ver outliers de todo o período  
   - `2` – ver outliers de um ano específico  
   - `3` – ver outliers de um determinado mês/ano  
   - `4` – sair (desliga o modo de interação e volta para “Sistema OFF”)

8. Os outliers são exibidos na tela em **páginas de 3 registros**. Quando há mais de 3, o sistema:

   - mostra 3 outliers  
   - pede para o usuário **pressionar qualquer tecla** para ver os próximos 3  
   - repete até mostrar todos

9. Um **botão de liga/desliga** alterna entre:
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

Código principal do ESP32 usando Arduino core, com:

inicialização do SD

cálculo da regressão

detecção de outliers

interface gráfica no TFT

leitura do keypad e botão liga/desliga

dados/medicoes.txt
Arquivo de texto com as medições reais ou simuladas.

imagens/esquematico.png
Diagrama de ligações entre ESP32, SD, ILI9341, keypad e botão.

3. Materiais utilizados
3.1. Hardware
1 × ESP32 DevKit v1

1 × Display TFT ILI9341 2.4" (SPI)

1 × Módulo de cartão SD (SPI)

1 × Teclado matricial 4×4 (wokwi-membrane-keypad)

1 × Botão push-button (liga/desliga)

1 × Cartão SD (real ou simulado no Wokwi)

Jumpers

Observação: o sensor de vazão não é usado diretamente. As medições de água já vêm prontas em medicoes.txt e são lidas do cartão SD.

3.2. Software / Bibliotecas
Arduino core for ESP32

Bibliotecas Arduino:

SPI.h

SD.h

Adafruit_GFX.h

Adafruit_ILI9341.h

Keypad.h

Wokwi (opcional) para simulação do circuito
https://wokwi.com/

4. Conexões do circuito
A seguir, um resumo das ligações principais entre ESP32, SD, TFT, keypad e botão.

4.1. Barramento SPI compartilhado (SD + Display)
O SD e o display ILI9341 compartilham o mesmo barramento SPI (VSPI do ESP32):

text
Copiar código
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

O código usa:

cpp
Copiar código
#define SD_CS  5
#define TFT_DC 2
#define TFT_CS 15
Adafruit_ILI9341 tft(TFT_CS, TFT_DC);
4.2. Teclado matricial 4×4 (wokwi-membrane-keypad)
Mapa de teclas:

text
Copiar código
1 2 3 A
4 5 6 B
7 8 9 C
* 0 # D
Ligação das linhas e colunas:

text
Copiar código
Teclado 4x4     ESP32
------------    -----
R1          ->  GPIO13
R2          ->  GPIO12
R3          ->  GPIO14
R4          ->  GPIO27
C1          ->  GPIO26
C2          ->  GPIO25
C3          ->  GPIO33
C4          ->  GPIO32
No código:

cpp
Copiar código
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
4.3. Botão Liga/Desliga
O botão usa o pull-up interno do ESP32:

text
Copiar código
Botão         ESP32
-------       -----
um terminal -> GPIO21
outro       -> GND
No código:

cpp
Copiar código
#define PWR_BTN_PIN 21
pinMode(PWR_BTN_PIN, INPUT_PULLUP);
Importante: pinos 34 a 39 do ESP32 são apenas entrada e não possuem pull-up interno.
Por isso o botão não funciona bem em pino 35 usando INPUT_PULLUP.
Neste projeto usamos o pino 21, que aceita pull-up interno e simplifica o circuito.

5. Técnica de detecção de anomalias
A parte matemática é a mesma do projeto original: regressão linear + resíduos + percentil 90.

5.1. Motivação
Se a bomba tem uma vazão aproximadamente constante (litros/minuto), então:

Quanto maior o tempo ligado, maior deve ser o volume bombeado.

Logo, as medições (tempo, litros) ficam próximas de uma reta.
Pontos muito distantes dessa reta são candidatos naturais a anomalias:

Muito tempo ligado para pouca água → possível falha de sucção, entrada de ar, poço baixo…

Muito volume em pouco tempo → possível erro de leitura, vazamento, manobra incomum…

5.2. Regressão linear
O código ajusta uma reta:

litros
≈
𝑎
⋅
duracao_min
+
𝑏
litros≈a⋅duracao_min+b
onde:

duracao_min: tempo de bombeamento (em minutos)

litros: volume bombeado no dia

a: coeficiente angular (litros/minuto)

b: intercepto

Passos:

Calcula as médias meanX (tempo) e meanY (litros).

Calcula:

s_xx = Σ (x_i - meanX)^2

s_xy = Σ (x_i - meanX)*(y_i - meanY)

Obtém:

a = s_xy / s_xx

b = meanY - a * meanX

Tudo isso é feito na função load_and_process_file().

5.3. Resíduos
Para cada medição:

Calcula o valor esperado pela reta:

c
Copiar código
y_pred = a * duracao_min + b;
Calcula o resíduo:

c
Copiar código
residuo = litros_medidos - y_pred;
Guarda também o valor absoluto:

c
Copiar código
abs_residuo = fabsf(residuo);
5.4. Percentil 90
Copia todos os abs_residuo para um vetor auxiliar

Ordena o vetor em ordem crescente

Pega o elemento no índice:

c
Copiar código
idx = 0.9f * (gCount - 1);
Esse valor é o limiar (gThrAbsResid):

Se abs_residuo > gThrAbsResid → medição marcada como outlier

Caso contrário → normal

6. Interface com TFT + Keypad
6.1. Organização da tela
O display é inicializado com:

cpp
Copiar código
tft.begin();
tft.setRotation(1);   // horizontal (paisagem)
A função tftClear():

limpa a tela com fillScreen(ILI9341_BLACK)

posiciona o cursor no canto superior esquerdo

define cor e tamanho padrão de texto

Todos os menus e mensagens partem dessa função para manter um layout simples e legível.

6.2. Botão de liga/desliga
A lógica de energia é controlada por uma variável booleana systemOn:

systemOn = false → “Sistema OFF”, só mostra mensagem pedindo POWER

systemOn = true → exibe menu e aceita entrada do keypad

A função updatePowerButton():

Lê digitalRead(PWR_BTN_PIN).

Detecta a borda de descida (HIGH → LOW).

Alterna systemOn e atualiza a tela:

Sistema ON

Sistema OFF / POWER p/ ligar

Aplica um pequeno delay(200) para debounce.

6.3. Leitura do keypad
O keypad usa a biblioteca Keypad.h. No menu:

cpp
Copiar código
int readMenuOption() {
  while (true) {
    updatePowerButton();
    if (!systemOn) return -1;

    char k = keypad.getKey();
    if (k >= '1' && k <= '4') {
      tft.print(k);
      return k - '0';
    }
    delay(10);
  }
}
Para o menu principal, o usuário digita 1, 2, 3 ou 4.

Para digitar um número inteiro (ano/mês), usa-se readIntFromKeypad():

dígitos 0–9 vão sendo acumulados

* limpa a entrada

# confirma (ENTER)

6.4. Paginação dos outliers (3 por tela)
As funções show_outliers_all(), show_outliers_year() e show_outliers_month_year():

Zera um contador shownOnPage.

Para cada outlier encontrado:

imprime os dados do dia (print_alert(i))

incrementa shownOnPage

Quando shownOnPage == 3:

chama waitAnyKey("Tecle p/ mais...")

limpa a tela

imprime de novo o título da tela

zera shownOnPage

Ao final, se nenhum outlier foi encontrado, imprime “Nenhum outlier.”.

Isso evita que a tela “role” para fora da área visível e permite navegar por muitos outliers com conforto.

7. Funcionamento do programa (fluxo setup() e loop())
7.1. setup()
Principais etapas:

Serial.begin(115200); (para debug, se desejado)

Configura o botão POWER:

cpp
Copiar código
pinMode(PWR_BTN_PIN, INPUT_PULLUP);
Inicializa o TFT (tft.begin(); tft.setRotation(1);) e mostra “Inicializando…”.

Inicializa o SD:

cpp
Copiar código
if (!SD.begin(SD_CS)) {
  tft.println("Falha SD.begin()");
  while (1) { delay(1000); }
}
Chama load_and_process_file(); para:

ler medicoes.txt

calcular regressão, resíduos, limiar e outliers

Finaliza mostrando:

txt
Copiar código
Sistema OFF
POWER p/ ligar
7.2. loop()
Fluxo principal:

Chama updatePowerButton();

Se systemOn == false, só faz delay(50); e volta (espera o botão).

Se systemOn == true:

exibe o menu principal:

txt
Copiar código
MENU OUTLIERS

1 - Todos
2 - Por ano
3 - Mes/ano
4 - Sair

Escolha (1-4):
lê a opção com readMenuOption().

De acordo com a opção:

1 → show_outliers_all();

2 → lê o ano pelo keypad, depois show_outliers_year(ano);

3 → lê mês e ano, depois show_outliers_month_year(mes, ano);

4 → mostra “Saindo…” e seta systemOn = false.

Após as opções 1–3, chama:

cpp
Copiar código
waitKeyToContinue("Fim. # p/ menu");
para o usuário voltar ao menu apertando #.

8. Como rodar o projeto
8.1. Preparar o arquivo medicoes.txt
Formato de cada linha:

txt
Copiar código
DD/MM/AAAA 000123.456789L HH:MM:SS
Exemplo:

txt
Copiar código
01/01/2025 000320.000000L 01:00:00
02/01/2025 000150.500000L 00:30:00
Coloque esse arquivo na raiz do cartão SD.

8.2. Arduino IDE + ESP32 físico
Configure a placa ESP32 (ex.: ESP32 Dev Module) no Arduino IDE.

Instale as bibliotecas:

Adafruit_GFX

Adafruit_ILI9341

Keypad

Copie src/outliers_tft_keypad.ino para uma nova sketch.

Monte o circuito conforme a seção 4. Conexões.

Grave o sketch no ESP32.

Insira o cartão SD com medicoes.txt.

Ligue o sistema:

A tela mostrará “Sistema OFF / POWER p/ ligar”.

Aperte o botão POWER para entrar no menu.

8.3. Simulação no Wokwi
Crie um novo projeto ESP32 (Arduino) no Wokwi.

Adicione:

ESP32

SD Card (type: "sdcard")

ILI9341

Membrane Keypad (4×4)

Push-button

Conecte os componentes de acordo com os pinos descritos na seção 4.

Adicione o arquivo medicoes.txt ao SD virtual (raiz).

Cole o código .ino no editor do Wokwi.

Inicie a simulação:

veja a tela inicial no ILI9341

aperte o botão para ligar

use o keypad para navegar pelo menu e pelas páginas de outliers.

9. Trabalhos futuros
Sensor de vazão físico: substituir os dados de arquivo por leitura em tempo real.

Gráficos na própria tela TFT: desenhar o gráfico tempo × litros com os pontos outliers destacados.

Conectividade Wi-Fi: enviar alertas para um servidor web/app sempre que forem detectados outliers.

Configuração pelo usuário: permitir alterar o limiar (percentil 90, 95 etc.) pela própria interface do display.

Armazenamento de novos registros: permitir que o ESP32 grave novas medições no SD a cada dia, mantendo o histórico atualizado.

Este projeto mostra como técnicas simples de estatística combinadas com um microcontrolador acessível podem apoiar a gestão de recursos hídricos em comunidades rurais, oferecendo uma ferramenta prática para identificar dias “estranhos” no funcionamento do sistema de bombeamento de água.
