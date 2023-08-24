#include <Arduino.h>
#define AA_FONT_SMALL "NotoSansBold15"
#define AA_FONT_LARGE "NotoSansBold36"
#include <LittleFS.h>
#define FileSys LittleFS
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include <PNGdec.h>
#include <HardwareSerial.h>
#include <JQ6500_Serial.h>
HardwareSerial mySerial(1);
JQ6500_Serial mp3(1);
// enum sound file names: AAfast.mp3, AAWas.mp3, AllesLiebe.mp3, BiBupnichtZahlen.mp3, DasReichAuch.mp3, DerCompu-Falsch.mp3, esWirdWarmer.mp3, FeuerFrei.mp3, heiss.mp3, HuiiKnapp.mp3, huiKnapp.mp3, LeiderAchRichtig.mp3, ManyManyManyWo.mp3, mEga.mp3, Neeein.mp3, Neiin.mp3, Nice.mp3, nope.mp3, OhJeah.mp3, RichtigGeil.mp3, sovielgeld.mp3, Uiuiuiuiui.mp3, wasDasWarsSchon.mp3, Wiebidde.mp3, wiederVersuchen.mp3
enum sounds {AAfast, AAWas, AllesLiebe, BiBupnichtZahlen, DasReichAuch, DerCompuFalsch, esWirdWarmer, FeuerFrei, heiss, HuiiKnapp, huiKnapp, LeiderAchRichtig, ManyManyManyWo, mEga, Neeein, Neiin, Nice, nope, OhJeah, RichtigGeil, sovielgeld, Uiuiuiuiui, wasDasWarsSchon, Wiebidde, wiederVersuchen};
//liste okay Sounds
sounds okaySounds[7] = {AAWas, AllesLiebe, Nice, OhJeah, RichtigGeil, LeiderAchRichtig, mEga};
//liste falsch Sounds
sounds falschSounds[14] = {AAfast, AAWas, BiBupnichtZahlen, DerCompuFalsch, esWirdWarmer, heiss, HuiiKnapp, huiKnapp, Neeein, Neiin, nope, Uiuiuiuiui, Wiebidde, wiederVersuchen};
int anzahl_okaySounds = 7;
int aktuell_okaySound = -1;
int anzahl_falschSounds = 14;
int aktuell_falschSound = -1;

int getOkSound() {
  aktuell_okaySound = aktuell_okaySound+1;
  if (aktuell_okaySound >= anzahl_okaySounds) {
    aktuell_okaySound = 0;
  }
  return okaySounds[aktuell_okaySound];
}

int getFalschSound() {
  aktuell_falschSound = aktuell_falschSound+1;
  if (aktuell_falschSound >= anzahl_falschSounds) {
    aktuell_falschSound = 0;
  }
  return falschSounds[aktuell_falschSound];
}


File pngfile;
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
enum status {START, IDLE, PINFELD, PIN_FALSCH, PIN_VERGESSEN, FRAGEN, FRAGE_RICHTIG, FRAGE_FALSCH, CAPTCHA, KONTO, GELDAUSWURF, ENDE};
status aktuellerStatus = START;
int aktuelleFrage = 0;
int captcha_counter = 0;
//BG Color
uint16_t bg_color = 0x3758;//0x471A;

//Fragen
int anzahl_fragen = 6;
String fragen[6] = {"An welchem Tag seid ihr zusammen gekommen?        (DDMMYYYY)", "Wann habt ihr die Schule abgeschlossen? (YYYY)", "Wann kam euer 2. Geborenes zur Welt?     (TTMMYYY)", "Wie viele Jahre seid ihr zusammen?", "In welchen Jahr seid ihr in euer jetziges Haus gezogen?", "Wann habt ihr euch verlobt? (DDMMYYYY)"};
String antworten[6] = {"25042010", "2014", "20022016", "13", "2021", "30092022"};
//Puffer für die Antworten
String antwortPuffer = "          ";
String pin = "5 2 3 4 ";

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

//MOTOR
const int motorPin = 27; // PWM-Pin für den Motor
const int frequency = 5000; // PWM-Frequenz in Hz (5kHz)
const int channel = 0; // PWM-Kanal für den ESP32
const int resolution = 8; // 8-bit Auflösung für den PWM, das gibt Werte zwischen 0-255


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

  // Initialisierung des PWM-Ausgangs
  ledcSetup(channel, frequency, resolution);
  ledcAttachPin(motorPin, channel);

  tft.begin();

  tft.setRotation(3);
  tft.fillScreen(bg_color); // Clear screen to navy background
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
mySerial.begin(9600, SERIAL_8N1, 33, 32);
 mp3.begin(9600);
  mp3.reset();
  delay(100);
  mp3.setVolume(30);
  delay(100);
  mp3.setLoopMode(MP3_LOOP_NONE);
  //mp3.playFileByIndexNumber(1); 
}

void setMotorSpeed(int speedPercentage) {
  if (speedPercentage < 0) speedPercentage = 0;
  if (speedPercentage > 100) speedPercentage = 100;

  // Umrechnung von Prozent in PWM-Wert (0-255)
  int pwmValue = map(speedPercentage, 0, 100, 0, 255);
  ledcWrite(channel, pwmValue);
}

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

void drawMyLogo(const char* name = "/logo.png") {
  // Pass support callback function names to library
  int16_t rc = png.open(name, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
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

// Funktion, to split to long lines
String splitToLines(String text, int maxLineLength) {
  String result = "";
  int lastSpace = -1;
  int lastLineStart = 0;
  for (int i = 0; i < text.length(); i++) {
    if (text.charAt(i) == ' ') {
      lastSpace = i;
    }
    if (i - lastLineStart > maxLineLength) {
      if (lastSpace == -1) {
        lastSpace = i;
      }
      result = result + text.substring(lastLineStart, lastSpace) + "\n  ";
      lastLineStart = lastSpace + 1;
      lastSpace = -1;
    }
  }
  result = result + text.substring(lastLineStart);
  return result;
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
              mp3.playFileByIndexNumber(getOkSound());
            } else {
              aktuellerStatus = PIN_FALSCH;
              aktuelleFrage = 0;
              redraw = true;
              antwortPuffer = "";
              mp3.playFileByIndexNumber(getFalschSound());
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
        } else if (aktuellerStatus == PIN_VERGESSEN) {
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
              aktuellerStatus = FRAGE_RICHTIG;
              if (aktuelleFrage == anzahl_fragen) {
                aktuellerStatus = KONTO;
              }
              mp3.playFileByIndexNumber(getOkSound());
            } else {
              aktuellerStatus = FRAGE_FALSCH;
              //aktuellerStatus = PIN_FALSCH;
              //aktuelleFrage = 0;
              redraw = true;
              antwortPuffer = "";
              mp3.playFileByIndexNumber(getFalschSound());
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
        } else if (aktuellerStatus == CAPTCHA) {
          if (keys[r][c] == 'E') {
              if (captcha_counter < anzahl_falschSounds) {
              captcha_counter = captcha_counter+1;
              redraw = true;
              antwortPuffer = "";
              mp3.playFileByIndexNumber(getFalschSound());
            } else {
              aktuellerStatus = KONTO;
              //aktuellerStatus = PIN_FALSCH;
              //aktuelleFrage = 0;
              redraw = true;
              antwortPuffer = "";
            }
          } else if (keys[r][c] == 'C') {
              redraw = true;
              antwortPuffer = "";
          } else {
            if (antwortPuffer.length() >= 4) {
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
    //tft.fillScreen(bg_color); // Clear screen to navy background
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
    //tft.fillScreen(TFT_BLACK); // Clear screen
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
      //drawMyLogo("/bg.png");
      tft.setCursor(20, 90, 2);
      tft.fillScreen(bg_color);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println("Bitte ihre PIN eingeben:");
      //tft.setTextWrap(false);
      //tft.setCursor(20, 100, 2);
      tft.println(" ");
      tft.print("                  ");
      tft.println(antwortPuffer.c_str());
      redraw = false;
    }
    /* code */
    break; 
  case PIN_FALSCH:
    if (redraw) {
      //drawMyLogo("/bg.png");
      tft.setCursor(120, 140, 2);
      tft.fillScreen(TFT_RED);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println("PIN falsch");
      //tft.setCursor(20, 100, 2);
      //tft.println("                   Nein = ESC");
      //tft.print("                    Ja = ENTER");
      //redraw = false;
      delay(2000);
      aktuellerStatus = PIN_VERGESSEN;
    }
    break;
  case PIN_VERGESSEN:
    if (redraw) {
      //drawMyLogo("/bg.png");
      tft.setCursor(110, 70, 2);
      tft.fillScreen(bg_color);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println("PIN vergessen?");
      //tft.setCursor(20, 100, 2);
      tft.println(" ");
      tft.println("              Nein = ESC");
      tft.print("               Ja = ENTER");
      redraw = false;
    }
    break;
  case FRAGEN:
    if (redraw) {
      //drawMyLogo("/bg.png");
      tft.setCursor(20, 70, 2);
      tft.fillScreen(bg_color);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println(splitToLines(fragen[aktuelleFrage], 23));
      //tft.setCursor(20, 100, 2);
      tft.println(" ");
      tft.print("                ");
      tft.print(antwortPuffer);
      redraw = false;
    }
    break;
  case FRAGE_FALSCH:
    if (redraw) {
      //drawMyLogo("/bg.png");
      tft.setCursor(100, 140, 2);
      tft.fillScreen(TFT_RED);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println("Antwort falsch");
      //tft.setCursor(20, 100, 2);
      //tft.println("                   Nein = ESC");
      //tft.print("                    Ja = ENTER");
      //redraw = false;
      delay(2000);
      aktuellerStatus = FRAGEN;
    }
    break;
  case FRAGE_RICHTIG:
    if (redraw) {
      //drawMyLogo("/bg.png");
      tft.setCursor(120, 140, 2);
      tft.fillScreen(TFT_GREEN);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println("Antwort richtig");
      //tft.setCursor(20, 100, 2);
      //tft.println("                   Nein = ESC");
      //tft.print("                    Ja = ENTER");
      //redraw = false;
      delay(2000);
      aktuellerStatus = FRAGEN;
    }
    break;
  case CAPTCHA:
    if (redraw) {
      //drawMyLogo("/bg.png");
      tft.setCursor(5, 70, 2);
      tft.fillScreen(bg_color);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_SMALL, LittleFS); // Must load the font first
      tft.println(splitToLines("Sie waren zu schnell, wir müssen Überprüfen, ob sie ein Bot sind: Ich habe eine Random Zahl zwischen 1 und 100, da Bots kein Random können, ist das das ideale Cpathca. Wie ist die Zahl?", 80));
      //tft.setCursor(20, 100, 2);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      if (captcha_counter != 0) {
        tft.println("Antwort falsch");
      } else {
        tft.println(" ");
      }
      tft.print("                ");
      tft.print(antwortPuffer);
      redraw = false;
    }
    break;
  case KONTO:
    if (redraw) {
      //drawMyLogo("/bg.png");
      tft.setCursor(60, 70, 2);
      tft.fillScreen(bg_color);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println("Geld Auszahlen?");
      //tft.setCursor(20, 100, 2);
      tft.println(" ");
      tft.println("               Nein = ESC");
      tft.print("                Ja = ENTER");
      redraw = false;
    }
    break;
  case GELDAUSWURF:
  //drawMyLogo(); // MONEY IMAGE
    if (redraw) {
      //drawMyLogo("/bg.png");
      tft.setCursor(100, 70, 2);
      tft.fillScreen(bg_color);
      tft.setTextColor(TFT_WHITE);  // Set text colour to white and background to blue
      tft.setTextSize(4);
      tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println("Geld wird Vorbereitet!");
      //tft.setCursor(20, 100, 2);
      //tft.println("               VORSICHT");
      //tft.print("                SCHNELL");
      redraw = false;
      //MOTOR und warten und dann zu ende
      aktuellerStatus = ENDE;
      redraw = true;
      delay(10000);
      mp3.playFileByIndexNumber(FeuerFrei);
      delay(2000);
      setMotorSpeed(100);
      delay(8000);
      mp3.playFileByIndexNumber(wasDasWarsSchon);
      setMotorSpeed(0);
      delay(6000);
      mp3.playFileByIndexNumber(ManyManyManyWo);
      setMotorSpeed(100);
      delay(5000);
      mp3.playFileByIndexNumber(sovielgeld);
    }
    /* code */
    break;
  case ENDE:
    if (redraw) {
      //tft.fillScreen(bg_color);
    drawMyLogo("/bg.png");
    tft.setCursor(20, 70, 2);
    tft.loadFont(AA_FONT_LARGE, LittleFS); // Must load the font first
      tft.println(splitToLines("Eine tolle Hochzeit wuenschen euch Gina, Marco, Max, Jake und Andre", 23));
      redraw = false;
      delay(10000);
      setMotorSpeed(0);
    }
    break;
  default:
    break;
  }
}
