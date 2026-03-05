#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Fonts/Org_01.h>    // Include a different font
#include <EEPROM.h> 

/*______Define LCD pins______*/
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define BUTTON_PIN 14 

/*_______Colors______*/
#define BLACK   0x0000
#define RED     0xF800
#define GREEN   0x07E0
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define BLUE_SKY tft.color565(50, 50, 255)

int BLUE; 
#define DRAW_LOOP_INTERVAL 50  
#define EEPROM_SIZE 512
#define addr 0

int currentpage = 0;
int menuSelection = 0; 
int currentWing;        
int flX, flY, fallRate; 
int pillarPos, gapPosition;  
int score, highScore = 0;      
bool running = false;   
bool crashed = false;   
int currentpcolour;
long nextDrawLoopRunTime;

enum ButtonState { NONE, SHORT_PRESS, LONG_PRESS };

// Prototypes
void drawHome();
void startGame();
void drawLoop();
void checkCollision();
void clearPillar(int x, int gap);
void drawPillar(int x, int gap);
void clearFlappy(int x, int y);
void drawFlappy(int x, int y);
void drawWing1(int x, int y);
void drawWing2(int x, int y);
void drawWing3(int x, int y);

ButtonState checkButton() {
  static unsigned long pressTime = 0;
  static bool isPressed = false;
  static bool longPressHandled = false;
  bool currentState = digitalRead(BUTTON_PIN) == LOW; 
  if (currentState && !isPressed) {
    pressTime = millis(); isPressed = true; longPressHandled = false;
  } else if (currentState && isPressed) {
    if (!longPressHandled && (millis() - pressTime > 600)) {
      longPressHandled = true; return LONG_PRESS;
    }
  } else if (!currentState && isPressed) {
    isPressed = false;
    if (!longPressHandled && (millis() - pressTime > 50)) return SHORT_PRESS;
  }
  return NONE;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  EEPROM.begin(EEPROM_SIZE);

  tft.init(TFT_WIDTH, TFT_HEIGHT); 
  tft.setRotation(2); // Set to 2 for Vertical. If upside down, change to 0.
  tft.invertDisplay(true); 

  BLUE = BLUE_SKY;

  tft.fillScreen(BLACK);
  tft.setTextSize(2); // Reduced slightly for 240 width
  tft.setTextColor(WHITE);
  tft.setCursor(45, 40);
  tft.print("This Project");
  tft.setCursor(75, 80);
  tft.print("Done By");
  tft.setTextSize(1);
  tft.setTextColor(YELLOW);
  tft.setCursor(55, 120);
  tft.print("Teach Me Something");
  tft.setTextSize(2);
  tft.setCursor(60, 180);
  tft.setTextColor(GREEN);
  tft.print("Loading...");

  for (int i=0; i < 240; i+=2) {
    tft.fillRect(0, 240, i, 10, RED);
    delay(5);
  }
  drawHome();
}

void loop() {
  ButtonState btn = checkButton();
  switch (currentpage) {
    case 0:
      if (btn == SHORT_PRESS) { menuSelection = (menuSelection + 1) % 3; drawHome(); }
      else if (btn == LONG_PRESS && menuSelection == 0) { currentpage = 1; startGame(); }
      break;
    case 1:
      if (btn == LONG_PRESS) { currentpage = 0; drawHome(); }
      else if (btn == SHORT_PRESS) {
        if (crashed) startGame();
        else if (!running) { running = true; fallRate = -7; }
        else fallRate = -7;
      }
      if (millis() > nextDrawLoopRunTime && !crashed) {
        drawLoop();
        checkCollision();
        nextDrawLoopRunTime += DRAW_LOOP_INTERVAL;
      }
      break;
  }
}

void drawHome() {
  tft.fillScreen(BLACK);
  tft.drawRoundRect(0, 0, 239, 319, 8, WHITE);
  tft.setFont();
  tft.setCursor(45, 30);
  tft.setTextSize(3);
  tft.setTextColor(WHITE);
  tft.print("MENU");

  String options[3] = {"1. Flappy", "2. RGB", "3. OSC"};
  for(int i=0; i<3; i++) {
    int y = 90 + i*60;
    if(menuSelection == i) tft.fillRoundRect(20, y, 200, 45, 8, RED);
    else tft.fillRoundRect(20, y, 200, 45, 8, BLUE);
    tft.drawRoundRect(20, y, 200, 45, 8, WHITE);
    tft.setCursor(40, y + 15);
    tft.setTextSize(2);
    tft.print(options[i]);
  }
}

void startGame() {
  flX = 40; flY = 120; fallRate = 0;
  pillarPos = 240; gapPosition = 80;
  crashed = false; score = 0;
  highScore = EEPROM.read(addr);
  
  tft.setFont(&Org_01);
  tft.fillScreen(BLUE);
  tft.setTextColor(YELLOW); tft.setTextSize(2);
  tft.setCursor(10, 25); tft.print("Flappy Bird");
  tft.setTextColor(WHITE); tft.setTextSize(1);
  tft.setCursor(10, 45); tft.print("Press to flap!!");
  
  // Ground
  for (int tx = 0; tx <= 240; tx += 20) {
    tft.fillTriangle(tx, 300, tx + 10, 300, tx, 310, GREEN);
    tft.fillTriangle(tx + 10, 310, tx + 10, 300, tx, 310, YELLOW);
  }
  nextDrawLoopRunTime = millis() + DRAW_LOOP_INTERVAL;
  running = false;
}

void drawLoop() { 
  clearPillar(pillarPos, gapPosition);   
  clearFlappy(flX, flY);            

  if (running) {
    flY += fallRate;
    fallRate++;
    pillarPos -= 5;
    if (pillarPos < -50) {
      pillarPos = 240;
      gapPosition = random(40, 180); 
      score++;
    }
  }

  currentpcolour = (score >= highScore && score > 0) ? YELLOW : GREEN;
  drawPillar(pillarPos, gapPosition);   
  drawFlappy(flX, flY);            
  switch (currentWing) {     
    case 0: case 1: drawWing1(flX, flY); break;  
    case 2: case 3: drawWing2(flX, flY); break;  
    case 4: case 5: drawWing3(flX, flY); break;  
  }
  currentWing = (currentWing + 1) % 6;
}

void checkCollision() {
  if (flY > 276) crashed = true;
  if (flX + 30 > pillarPos && flX < pillarPos + 50) {
    if (flY < gapPosition || flY + 24 > gapPosition + 90) crashed = true;
  }
  if (crashed) {      
    tft.setFont();
    tft.setTextColor(RED); tft.setTextSize(3);      
    tft.setCursor(20, 130); tft.print("GAME OVER"); 
    tft.setTextSize(2); tft.setTextColor(WHITE);
    tft.setCursor(60, 170); tft.print("Score: "); tft.print(score);
    if (score > highScore) {           
      EEPROM.write(addr, score); EEPROM.commit();
    }
    running = false;      
  }
}

// --- YOUR ORIGINAL BIRD & PILLAR PIXEL ART ---
void drawPillar(int x, int gap) {  
  tft.fillRect(x + 2, 0, 46, gap, currentpcolour);
  tft.fillRect(x + 2, gap + 90, 46, 300 - (gap + 90), currentpcolour);
  tft.drawRect(x, 0, 50, gap, BLACK);
  tft.drawRect(x, gap + 90, 50, 300 - (gap + 90), BLACK);
}

void clearPillar(int x, int gap) {  
  tft.fillRect(x + 45, 0, 5, 300, BLUE);
}

void clearFlappy(int x, int y) { tft.fillRect(x, y, 34, 24, BLUE); }

void drawFlappy(int x, int y) {  
  tft.fillRect(x + 2, y + 8, 2, 10, BLACK);
  tft.fillRect(x + 4, y + 6, 2, 2, BLACK);
  tft.fillRect(x + 6, y + 4, 2, 2, BLACK);
  tft.fillRect(x + 8, y + 2, 4, 2, BLACK);
  tft.fillRect(x + 12, y, 12, 2, BLACK);
  tft.fillRect(x + 24, y + 2, 2, 2, BLACK);
  tft.fillRect(x + 26, y + 4, 2, 2, BLACK);
  tft.fillRect(x + 28, y + 6, 2, 6, BLACK);
  tft.fillRect(x + 10, y + 22, 10, 2, BLACK);
  tft.fillRect(x + 4, y + 18, 2, 2, BLACK);
  tft.fillRect(x + 6, y + 20, 4, 2, BLACK);
  tft.fillRect(x + 12, y + 2, 6, 2, YELLOW);
  tft.fillRect(x + 8, y + 4, 8, 2, YELLOW);
  tft.fillRect(x + 6, y + 6, 10, 2, YELLOW);
  tft.fillRect(x + 4, y + 8, 12, 2, YELLOW);
  tft.fillRect(x + 4, y + 10, 14, 2, YELLOW);
  tft.fillRect(x + 4, y + 12, 16, 2, YELLOW);
  tft.fillRect(x + 4, y + 14, 14, 2, YELLOW);
  tft.fillRect(x + 4, y + 16, 12, 2, YELLOW);
  tft.fillRect(x + 6, y + 18, 12, 2, YELLOW);
  tft.fillRect(x + 10, y + 20, 10, 2, YELLOW);
  tft.fillRect(x + 18, y + 2, 2, 2, BLACK);
  tft.fillRect(x + 16, y + 4, 2, 6, BLACK);
  tft.fillRect(x + 18, y + 10, 2, 2, BLACK);
  tft.fillRect(x + 18, y + 4, 2, 6, WHITE);
  tft.fillRect(x + 20, y + 2, 4, 10, WHITE);
  tft.fillRect(x + 24, y + 4, 2, 8, WHITE);
  tft.fillRect(x + 26, y + 6, 2, 6, WHITE);
  tft.fillRect(x + 24, y + 6, 2, 4, BLACK);
  tft.fillRect(x + 20, y + 12, 12, 2, BLACK);
  tft.fillRect(x + 18, y + 14, 2, 2, BLACK);
  tft.fillRect(x + 20, y + 14, 12, 2, RED);
  tft.fillRect(x + 32, y + 14, 2, 2, BLACK);
  tft.fillRect(x + 16, y + 16, 2, 2, BLACK);
  tft.fillRect(x + 18, y + 16, 2, 2, RED);
  tft.fillRect(x + 20, y + 16, 12, 2, BLACK);
  tft.fillRect(x + 18, y + 18, 2, 2, BLACK);
  tft.fillRect(x + 20, y + 18, 10, 2, RED);
  tft.fillRect(x + 30, y + 18, 2, 2, BLACK);
  tft.fillRect(x + 20, y + 20, 10, 2, BLACK);
}

void drawWing1(int x, int y) {
  tft.fillRect(x, y + 14, 2, 6, BLACK);
  tft.fillRect(x + 2, y + 20, 8, 2, BLACK);
  tft.fillRect(x + 2, y + 12, 10, 2, BLACK);
  tft.fillRect(x + 12, y + 14, 2, 2, BLACK);
  tft.fillRect(x + 10, y + 16, 2, 2, BLACK);
  tft.fillRect(x + 2, y + 14, 8, 6, WHITE);
  tft.fillRect(x + 8, y + 18, 2, 2, BLACK);
  tft.fillRect(x + 10, y + 14, 2, 2, WHITE);
}

void drawWing2(int x, int y) {
  tft.fillRect(x + 2, y + 10, 10, 2, BLACK);
  tft.fillRect(x + 2, y + 16, 10, 2, BLACK);
  tft.fillRect(x, y + 12, 2, 4, BLACK);
  tft.fillRect(x + 12, y + 12, 2, 4, BLACK);
  tft.fillRect(x + 2, y + 12, 10, 4, WHITE);
}

void drawWing3(int x, int y) {
  tft.fillRect(x + 2, y + 6, 8, 2, BLACK);
  tft.fillRect(x, y + 8, 2, 6, BLACK);
  tft.fillRect(x + 10, y + 8, 2, 2, BLACK);
  tft.fillRect(x + 12, y + 10, 2, 4, BLACK);
  tft.fillRect(x + 10, y + 14, 2, 2, BLACK);
  tft.fillRect(x + 2, y + 14, 2, 2, BLACK);
  tft.fillRect(x + 4, y + 16, 6, 2, BLACK);
  tft.fillRect(x + 2, y + 8, 8, 6, WHITE);
  tft.fillRect(x + 4, y + 14, 6, 2, WHITE);
  tft.fillRect(x + 10, y + 10, 2, 4, WHITE);
}