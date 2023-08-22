#include <Arduino.h>
#define AA_FONT_SMALL "NotoSansBold15"
#define AA_FONT_LARGE "NotoSansBold36"
#include <LittleFS.h>
#define FileSys LittleFS
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include <PNGdec.h>
PNG png;
#define MAX_IMAGE_WIDTH 480 // Adjust for your images
int16_t xpos = 0;
int16_t ypos = 0;
bool redraw = true;

//last key
char lastKey = ' ';

TFT_eSPI tft = TFT_eSPI();                   // Invoke custom library with default width and height

unsigned long drawTime = 0;
//ENUM status, Start, IDLE, Pin Feld, Fragen, Konto, Geldauswurf, ENDE
enum status {START, IDLE, PINFELD, PIN_FALSCH, FRAGEN, KONTO, GELDAUSWURF, ENDE};
status aktuellerStatus = START;
int aktuelleFrage = 0;

//Fragen
String fragen[5] = {"Wann seid ihr nach Luebeck gezogen?", "Wann habt ihr euren Schulabschluss gemacht?", "Wann kam euer 2. Gebohrenes zur Welt? (TTMMYYY)", "Wie viele Jahre seid ihr zusammen?", "In welchen Jahr seid ihr in euer jetziges Haus gezogen?"};
String antworten[5] = {"2014", "2014", "20022016", "13", "2021"};
//Puffer für die Antworten
String antwortPuffer = "          ";
String pin = "1 2 3 4 ";

// Pins
#define KARTENPIN 5
#define LEDPIN 4

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
  // LED Displaybeleuchtung
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);

  // Karten Pin
  pinMode(KARTENPIN, INPUT_PULLUP);

  // Initialisiere die Reihenpins als Eingänge
  for (int r = 0; r < 4; r++) {
    pinMode(rowPins[r], INPUT);
  }

  tft.begin();

  tft.setRotation(3);
  tft.fillScreen(TFT_NAVY); // Clear screen to navy background
  // Initialise FS
  if (!FileSys.begin()) {
    Serial.println("LittleFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  bool font_missing = false;
  if (LittleFS.exists("/NotoSansBold15.vlw")    == false) font_missing = true;
  if (LittleFS.exists("/NotoSansBold36.vlw")    == false) font_missing = true;

  if (font_missing)
  {
    Serial.println("\nFont missing in Flash FS, did you upload it?");
    while(1) yield();
  }
  else Serial.println("\nFonts found OK.");
}


File pngfile;

void * pngOpen(const char *filename, int32_t *size) {
  Serial.printf("Attempting to open %s\n", filename);
  pngfile = FileSys.open(filename, "r");
  *size = pngfile.size();
  return &pngfile;
}

void pngClose(void *handle) {
  File pngfile = *((File*)handle);
  if (pngfile) pngfile.close();
}

int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length) {
  if (!pngfile) return 0;
  page = page; // Avoid warning
  return pngfile.read(buffer, length);
}

int32_t pngSeek(PNGFILE *page, int32_t position) {
  if (!pngfile) return 0;
  page = page; // Avoid warning
  return pngfile.seek(position);
}

//=========================================v==========================================
//                                      pngDraw
//====================================================================================
// This next function will be called during decoding of the png file to
// render each image line to the TFT.  If you use a different TFT library
// you will need to adapt this function to suit.
// Callback function to draw pixels to the display
void pngDraw(PNGDRAW *pDraw) {
  uint16_t lineBuffer[MAX_IMAGE_WIDTH];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

void drawMyLogo() {
  // Pass support callback function names to library
  int16_t rc = png.open("/logo.png", pngOpen, pngClose, pngRead, pngSeek, pngDraw);
  if (rc == PNG_SUCCESS) {
    tft.startWrite();
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    uint32_t dt = millis();
    if (png.getWidth() > MAX_IMAGE_WIDTH) {
      Serial.println("Image too wide for allocated line buffer size!");
    }
    else {
      rc = png.decode(NULL, 0);
      png.close();
    }
    tft.endWrite();
    // How long did rendering take...
    Serial.print(millis()-dt); Serial.println("ms");
  }
}

void loop() {
  // Prüfen Sie, ob eine Taste gedrückt wurde und welche Taste es war
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
        if (aktuellerStatus == PINFELD) {
          if (keys[r][c] == 'E') {
            if (antwortPuffer == pin) {
              aktuellerStatus = KONTO;
              aktuelleFrage = 0;
              redraw = true;
              antwortPuffer = "";
            } else {
              aktuellerStatus = PIN_FALSCH;
              aktuelleFrage = 0;
              redraw = true;
              antwortPuffer = "";
            }
            redraw = true;
            antwortPuffer = "";
          } else if (keys[r][c] == 'C') {
            antwortPuffer = "";
            redraw = true;
            aktuellerStatus = IDLE;
          } else {
            if (antwortPuffer.length() >= 7) {
              Serial.println("Puffer länge: " + String(antwortPuffer.length()));
              antwortPuffer = "";
              Serial.println("Puffer gelöscht");
            }
            antwortPuffer = antwortPuffer + String(keys[r][c]) + " ";
            redraw = true;
          }
        } else if (aktuellerStatus == PIN_FALSCH) {
          if (keys[r][c] == 'E') {
              aktuellerStatus = FRAGEN;
              aktuelleFrage = 0;
              redraw = true;
              antwortPuffer = "";
          } else if (keys[r][c] == 'C') {
            aktuellerStatus = IDLE;
              aktuelleFrage = 0;
              redraw = true;
              antwortPuffer = "";
          }
        } else if (aktuellerStatus == FRAGEN) {
          if (keys[r][c] == 'E') {
              if (antwortPuffer == antworten[aktuelleFrage]) {
              aktuelleFrage = aktuelleFrage+1;
              redraw = true;
              antwortPuffer = "";
              if (aktuelleFrage == 5) {
                aktuellerStatus = KONTO;
              }
            } else {
              aktuellerStatus = PIN_FALSCH;
              aktuelleFrage = 0;
              redraw = true;
              antwortPuffer = "";
            }
          } else if (keys[r][c] == 'C') {
              redraw = true;
              antwortPuffer = "";
          } else {
            if (antwortPuffer.length() >= 10) {
              Serial.println("Puffer länge: " + String(antwortPuffer.length()));
              antwortPuffer = "";
              Serial.println("Puffer gelöscht");
            }
            antwortPuffer = antwortPuffer + String(keys[r][c]);
            redraw = true;
          }
        } else if (aktuellerStatus == KONTO) {
          if (keys[r][c] == 'E') {
              aktuellerStatus = GELDAUSWURF;
              aktuelleFrage = 0;
              redraw = true;
              antwortPuffer = "";
          } else if (keys[r][c] == 'C') {
            aktuellerStatus = IDLE;
              aktuelleFrage = 0;
              redraw = true;
              antwortPuffer = "";
          }
        }
        //tft.fillScreen(TFT_BLUE); // Clear screen to navy background
        //tft.setCursor(20, 20, 2);
        //tft.setTextColor(TFT_WHITE, TFT_BLUE);  // Set text colour to white and background to blue
        //tft.setTextSize(10);
        //tft.println(keys[r][c]);
      }
    }

    digitalWrite(colPins[c], HIGH);  // Deaktiviere die aktuelle Spalte
  }
  if (!keyIsPressed) {
    lastKey = ' ';
    //tft.fillScreen(TFT_NAVY); // Clear screen to navy background
  }
  if (digitalRead(KARTENPIN) == LOW) {
    //Serial.println("Karte eingeführt");
    if (aktuellerStatus == START || aktuellerStatus == IDLE) {
      aktuellerStatus = PINFELD;
      redraw = true;
      antwortPuffer = "_ _ _ _";
    }
  } else {
    //Serial.println("Keine Karte");
    aktuellerStatus = IDLE;
    aktuelleFrage = 0;
    redraw = true;
    antwortPuffer = "";
  }
  // nach status entscheiden, wa sgemacht wird mit einem stwitch case
  switch (aktuellerStatus) {
  case START:
    tft.fillScreen(TFT_BLACK); // Clear screen
    /* code */
    break;
  case IDLE:
    if (redraw) {
      drawMyLogo();
      redraw = false;
    }
    break;
  case PINFELD:
    if (redraw) {
      //drawMyLogo();
      tft.setCursor(20, 70, 2);
      tft.fillScreen(TFT_NAVY);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println("Bitte ihre PIN eingeben:");
      //tft.setCursor(20, 100, 2);
      tft.println(" ");
      tft.print("                    ");
      tft.println(antwortPuffer.c_str());
      redraw = false;
    }
    /* code */
    break; 
  case PIN_FALSCH:
    if (redraw) {
      //drawMyLogo();
      tft.setCursor(20, 70, 2);
      tft.fillScreen(TFT_NAVY);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println("PIN vergessen?");
      //tft.setCursor(20, 100, 2);
      tft.println("                   Nein = ESC");
      tft.print("                    Ja = ENTER");
      redraw = false;
    }
  case FRAGEN:
    if (redraw) {
      //drawMyLogo();
      tft.setCursor(20, 70, 2);
      tft.fillScreen(TFT_NAVY);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println(fragen[aktuelleFrage]);
      //tft.setCursor(20, 100, 2);
      tft.println(" ");
      tft.print("                    ");
      tft.print(antwortPuffer);
      redraw = false;
    }
    break;
  case KONTO:
    if (redraw) {
      //drawMyLogo();
      tft.setCursor(20, 70, 2);
      tft.fillScreen(TFT_NAVY);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println("Geld Auszahlen?");
      //tft.setCursor(20, 100, 2);
      tft.println("                   Nein = ESC");
      tft.print("                    Ja = ENTER");
      redraw = false;
    }
    break;
  case GELDAUSWURF:
  //drawMyLogo(); // MONEY IMAGE
    if (redraw) {
      //drawMyLogo();
      tft.setCursor(20, 70, 2);
      tft.fillScreen(TFT_NAVY);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println("Geld kommt!");
      //tft.setCursor(20, 100, 2);
      tft.println("                   VORSICHT");
      tft.print("                    SCHNELL");
      redraw = false;
      //MOTOR und warten und dann zu ende
      aktuellerStatus = ENDE;
      redraw = true;
      delay(5000);
    }
    /* code */
    break;
  case ENDE:
    if (redraw) {
      tft.fillScreen(TFT_NAVY);
    //drawMyLogo(); Bild von uns
    tft.setCursor(20, 70, 2);
    tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println("Eine tolle Hochzeit wuenschen euch Gina, Marco, Max, Jake und Andre");
      redraw = false;
    }
    break;
  default:
    break;
  }
}
