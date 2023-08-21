#include <Arduino.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

//last key
char lastKey = ' ';

TFT_eSPI tft = TFT_eSPI();                   // Invoke custom library with default width and height

unsigned long drawTime = 0;


// Definieren Sie die Pins
const int rowPins[4] = {39, 36, 35, 34};  // Reihe 4i, 3i, 2i, 1i
const int colPins[4] = {25, 16, 13, 12};  // Spalte 4O, 3O, 2O, 1O

// Definieren Sie das Tastenlayout
char keys[4][4] = {
  {'E', 'D', 'U', 'C'},
  {'#', '9', '6', '3'},
  {'0', '8', '5', '2'},
  {'*', '7', '4', '1'}
};

void setup() {
  Serial.begin(115200);

  // Initialisiere die Spaltenpins als Ausgänge
  for (int c = 0; c < 4; c++) {
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c], HIGH);
  }

  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  // Initialisiere die Reihenpins als Eingänge
  for (int r = 0; r < 4; r++) {
    pinMode(rowPins[r], INPUT);
  }

  tft.begin();

  tft.setRotation(1);
  tft.fillScreen(TFT_NAVY); // Clear screen to navy background
}

void loop() {
  bool keyIsPressed = false;
  for (int c = 0; c < 4; c++) {
    digitalWrite(colPins[c], LOW);  // Aktiviere die aktuelle Spalte

    for (int r = 0; r < 4; r++) {
      if (digitalRead(rowPins[r]) == LOW) {  // Wenn die Taste gedrückt wird, wird sie LOW sein
        keyIsPressed = true;
        if (lastKey == keys[r][c]) {
          continue;
        }
        Serial.print("Taste gedrückt: ");
        Serial.println(keys[r][c]);
        lastKey = keys[r][c];
        delay(50);  // Entprellen
        tft.fillScreen(TFT_BLUE); // Clear screen to navy background
        tft.setCursor(20, 20, 2);
        tft.setTextColor(TFT_WHITE, TFT_BLUE);  // Set text colour to white and background to blue
        tft.setTextSize(10);
        tft.println(keys[r][c]);
      }
    }

    digitalWrite(colPins[c], HIGH);  // Deaktiviere die aktuelle Spalte
  }
  if (!keyIsPressed) {
    lastKey = ' ';
    tft.fillScreen(TFT_NAVY); // Clear screen to navy background
  }
}
