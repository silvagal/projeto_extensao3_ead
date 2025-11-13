#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Keypad.h>
#include <math.h>

#define MAX_RECORDS 300

// ==== SD CARD ====
#define SD_CS   5     // CS do módulo SD
// SPI padrão ESP32 (VSPI): SCK=18, MISO=19, MOSI=23

// ==== ILI9341 ====
#define TFT_DC  2
#define TFT_CS  15
Adafruit_ILI9341 tft(TFT_CS, TFT_DC);

// ==== Botão liga/desliga ====
// NÃO usar 34–39 se quiser INPUT_PULLUP!
#define PWR_BTN_PIN 21

bool systemOn = false;
bool lastBtnState = HIGH;

// ==== Keypad 4x4 ==== 
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {13, 12, 14, 27};   // R1, R2, R3, R4
byte colPins[COLS] = {26, 25, 33, 32};   // C1, C2, C3, C4

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ==== Dados em memória ====
int   gDay[MAX_RECORDS];
int   gMonth[MAX_RECORDS];
int   gYear[MAX_RECORDS];
float gLitros[MAX_RECORDS];
float gDuracaoMin[MAX_RECORDS];
char  gTempoStr[MAX_RECORDS][9];  // "HH:MM:SS"

float gResid[MAX_RECORDS];
float gAbsResid[MAX_RECORDS];
int   gIsOutlier[MAX_RECORDS];

int   gCount       = 0;
float gSlope       = 0.0f;
float gIntercept   = 0.0f;
float gThrAbsResid = 0.0f;

// ==== Funções auxiliares de display ====
void tftClear() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
}

void waitKeyToContinue(const char* msg) {
  tft.println();
  tft.println(msg);
  tft.println("Pressione #");
  while (true) {
    char k = keypad.getKey();
    if (k == '#') {
      break;
    }
    delay(10);
  }
}

// espera QUALQUER tecla
void waitAnyKey(const char* msg) {
  tft.println();
  tft.println(msg);
  while (true) {
    char k = keypad.getKey();
    if (k) {
      break;
    }
    delay(10);
  }
}

// Atualiza estado do botão liga/desliga
void updatePowerButton() {
  bool state = digitalRead(PWR_BTN_PIN);
  if (lastBtnState == HIGH && state == LOW) {
    systemOn = !systemOn;
    tftClear();
    if (systemOn) {
      tft.println("Sistema ON");
    } else {
      tft.println("Sistema OFF");
      tft.println("POWER p/ ligar");
    }
    delay(200); // debounce simples
  }
  lastBtnState = state;
}

// Ler opção simples (1–4) do keypad
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

// Lê um int do keypad, com no máximo maxDigits, usando '#' como ENTER
int readIntFromKeypad(byte maxDigits, const char* prompt) {
  tftClear();
  tft.println(prompt);
  tft.println("Use '#' = ENTER");
  tft.println();
  tft.print(">> ");

  String buf = "";

  while (true) {
    updatePowerButton();
    if (!systemOn) return -1;

    char k = keypad.getKey();
    if (!k) {
      delay(10);
      continue;
    }

    if (k >= '0' && k <= '9') {
      if (buf.length() < maxDigits) {
        buf += k;
        tft.print(k);
      }
    } else if (k == '*') {
      // limpar entrada
      buf = "";
      tftClear();
      tft.println(prompt);
      tft.println("Use '#' = ENTER");
      tft.println();
      tft.print(">> ");
    } else if (k == '#') {
      if (buf.length() > 0) {
        return buf.toInt();
      }
    }
  }
}

// ==== Exibição de outliers no TFT ====
void print_alert(int i) {
  char line[64];

  snprintf(line, sizeof(line), "Dia: %02d/%02d/%04d",
           gDay[i], gMonth[i], gYear[i]);
  tft.println(line);

  snprintf(line, sizeof(line), "Dur: %s  L: %.2f",
           gTempoStr[i], gLitros[i]);
  tft.println(line);
  tft.println("");
}

// mostra com paginação (3 por tela)
void show_outliers_all() {
  tftClear();
  tft.println("Outliers - TODOS:");
  tft.println("");

  int found = 0;
  int shownOnPage = 0;

  for (int i = 0; i < gCount; i++) {
    if (!gIsOutlier[i]) continue;

    found++;
    print_alert(i);
    shownOnPage++;

    if (shownOnPage == 3) {
      waitAnyKey("Tecle p/ mais...");
      tftClear();
      tft.println("Outliers - TODOS:");
      tft.println("");
      shownOnPage = 0;
    }
  }

  if (!found) {
    tft.println("Nenhum outlier.");
  }
}

void show_outliers_year(int year) {
  tftClear();
  char line[32];
  snprintf(line, sizeof(line), "Outliers ano %d", year);
  tft.println(line);
  tft.println("");

  int found = 0;
  int shownOnPage = 0;

  for (int i = 0; i < gCount; i++) {
    if (gIsOutlier[i] && gYear[i] == year) {
      found++;
      print_alert(i);
      shownOnPage++;

      if (shownOnPage == 3) {
        waitAnyKey("Tecle p/ mais...");
        tftClear();
        tft.println(line);
        tft.println("");
        shownOnPage = 0;
      }
    }
  }
  if (!found) {
    tft.println("Nenhum outlier.");
  }
}

void show_outliers_month_year(int month, int year) {
  tftClear();
  char title[32];
  snprintf(title, sizeof(title), "Outliers %02d/%04d", month, year);
  tft.println(title);
  tft.println("");

  int found = 0;
  int shownOnPage = 0;

  for (int i = 0; i < gCount; i++) {
    if (gIsOutlier[i] &&
        gYear[i] == year &&
        gMonth[i] == month) {

      found++;
      print_alert(i);
      shownOnPage++;

      if (shownOnPage == 3) {
        waitAnyKey("Tecle p/ mais...");
        tftClear();
        tft.println(title);
        tft.println("");
        shownOnPage = 0;
      }
    }
  }
  if (!found) {
    tft.println("Nenhum outlier.");
  }
}

// ==== Leitura do arquivo + regressão + outliers ====
void load_and_process_file() {
  File f = SD.open("/medicoes.txt", FILE_READ);
  if (!f) {
    tftClear();
    tft.println("Erro ao abrir");
    tft.println("/medicoes.txt");
    while (1) {
      delay(1000);
    }
  }

  gCount = 0;
  while (f.available() && gCount < MAX_RECORDS) {
    String lineStr = f.readStringUntil('\n');
    if (lineStr.length() == 0) continue;

    char line[128];
    lineStr.trim();
    if (lineStr.length() >= (int)sizeof(line)) continue;
    lineStr.toCharArray(line, sizeof(line));

    char dateStr[16], litrosStr[32], tempoStr[16];
    if (sscanf(line, "%15s %31s %15s", dateStr, litrosStr, tempoStr) != 3) {
      continue;
    }

    size_t lenL = strlen(litrosStr);
    if (lenL > 0 && litrosStr[lenL - 1] == 'L') {
      litrosStr[lenL - 1] = '\0';
    }

    int d = 0, m = 0, y = 0;
    if (sscanf(dateStr, "%d/%d/%d", &d, &m, &y) != 3) {
      continue;
    }

    int hh = 0, mm = 0, ss = 0;
    if (sscanf(tempoStr, "%d:%d:%d", &hh, &mm, &ss) != 3) {
      continue;
    }

    float litros = atof(litrosStr);
    long totalSeconds = hh * 3600L + mm * 60L + ss;
    float durMin = totalSeconds / 60.0f;
    if (durMin <= 0.0f) {
      continue;
    }

    gDay[gCount]        = d;
    gMonth[gCount]      = m;
    gYear[gCount]       = y;
    gLitros[gCount]     = litros;
    gDuracaoMin[gCount] = durMin;

    strncpy(gTempoStr[gCount], tempoStr, sizeof(gTempoStr[gCount]) - 1);
    gTempoStr[gCount][sizeof(gTempoStr[gCount]) - 1] = '\0';

    gCount++;
  }
  f.close();

  if (gCount == 0) {
    tftClear();
    tft.println("Nenhuma medicao");
    tft.println("valida.");
    while (1) {
      delay(1000);
    }
  }

  // Regressão linear
  double sumX = 0.0, sumY = 0.0;
  for (int i = 0; i < gCount; i++) {
    sumX += gDuracaoMin[i];
    sumY += gLitros[i];
  }
  double meanX = sumX / gCount;
  double meanY = sumY / gCount;

  double s_xx = 0.0, s_xy = 0.0;
  for (int i = 0; i < gCount; i++) {
    double dx = gDuracaoMin[i] - meanX;
    double dy = gLitros[i]     - meanY;
    s_xx += dx * dx;
    s_xy += dx * dy;
  }

  if (s_xx == 0.0) {
    tftClear();
    tft.println("Erro regressao");
    while (1) {
      delay(1000);
    }
  }

  gSlope     = (float)(s_xy / s_xx);
  gIntercept = (float)(meanY - gSlope * meanX);

  for (int i = 0; i < gCount; i++) {
    float y_pred = gSlope * gDuracaoMin[i] + gIntercept;
    gResid[i]    = gLitros[i] - y_pred;
    gAbsResid[i] = fabsf(gResid[i]);
  }

  // Percentil 90 dos |residuos|
  float sortedAbs[MAX_RECORDS];
  for (int i = 0; i < gCount; i++) {
    sortedAbs[i] = gAbsResid[i];
  }

  for (int i = 0; i < gCount - 1; i++) {
    for (int j = i + 1; j < gCount; j++) {
      if (sortedAbs[j] < sortedAbs[i]) {
        float tmp   = sortedAbs[i];
        sortedAbs[i] = sortedAbs[j];
        sortedAbs[j] = tmp;
      }
    }
  }

  int idx = (int)(0.9f * (gCount - 1));
  if (idx < 0) idx = 0;
  if (idx >= gCount) idx = gCount - 1;
  gThrAbsResid = sortedAbs[idx];

  int qtdOut = 0;
  for (int i = 0; i < gCount; i++) {
    gIsOutlier[i] = (gAbsResid[i] > gThrAbsResid) ? 1 : 0;
    if (gIsOutlier[i]) qtdOut++;
  }

  tftClear();
  tft.println("Medicoes: " + String(gCount));
  tft.print("a = ");
  tft.print(gSlope, 3);
  tft.println(" L/min");
  tft.print("b = ");
  tft.print(gIntercept, 3);
  tft.println(" L");
  tft.print("|res| > ");
  tft.print(gThrAbsResid, 2);
  tft.println(" L");
  tft.print("Outliers: ");
  tft.println(qtdOut);

  waitKeyToContinue("Calculo OK!");
}

// ==== setup e loop ====
void setup() {
  Serial.begin(115200);

  pinMode(PWR_BTN_PIN, INPUT_PULLUP);

  tft.begin();
  tft.setRotation(1);  // horizontal
  tftClear();
  tft.println("Inicializando...");

  if (!SD.begin(SD_CS)) {
    tft.println("Falha SD.begin()");
    while (1) {
      delay(1000);
    }
  }

  load_and_process_file();

  tftClear();
  tft.println("Sistema OFF");
  tft.println("POWER p/ ligar");
}

void loop() {
  updatePowerButton();

  if (!systemOn) {
    delay(50);
    return;
  }

  // Menu principal
  tftClear();
  tft.println("MENU OUTLIERS");
  tft.println("");
  tft.println("1 - Todos");
  tft.println("2 - Por ano");
  tft.println("3 - Mes/ano");
  tft.println("4 - Sair");
  tft.println("");
  tft.println("Escolha (1-4):");

  int opt = readMenuOption();
  if (opt == -1) return; // desligou no meio (POWER)

  if (opt == 1) {
    show_outliers_all();
    waitKeyToContinue("Fim. # p/ menu");
  } else if (opt == 2) {
    int year = readIntFromKeypad(4, "Ano (ex: 2025)");
    if (year > 0) {
      show_outliers_year(year);
      waitKeyToContinue("Fim. # p/ menu");
    }
  } else if (opt == 3) {
    int month = readIntFromKeypad(2, "Mes (1-12)");
    int year  = readIntFromKeypad(4, "Ano (ex: 2025)");
    if (month > 0 && year > 0) {
      show_outliers_month_year(month, year);
      waitKeyToContinue("Fim. # p/ menu");
    }
  } else if (opt == 4) {
    tftClear();
    tft.println("Saindo...");
    systemOn = false;
    delay(300);
  }
}