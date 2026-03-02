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

// Initialize ST7789
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

/*______Define Button Pin______*/
#define BUTTON_PIN 14 // Connect a button between GPIO 14 and GND

#define BAR_MINY 30
#define BAR_HEIGHT 250
#define BAR_WIDTH 30
#define REDBAR_MINX 50
#define GREENBAR_MINX 130
#define BLUEBAR_MINX 180

/*_______Assign names to colors______*/
#define BLACK   0x0000
#define DARKBLUE 0x0010
#define VIOLET  0x8888
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GOLD    0xFEA0
#define BROWN   0xA145
#define LIME    0x07E0

int BLUE; 

#define DRAW_LOOP_INTERVAL 50  
#define EEPROM_SIZE 512
#define addr 0

int currentpage = 0;
int menuSelection = 0;  // 0: Flappy, 1: RGB, 2: OSC

int currentWing;        
int flX, flY, fallRate; 
int pillarPos, gapPosition;  
int score;              
int highScore = 0;      
bool running = false;   
bool crashed = false;   
int currentpcolour;
long nextDrawLoopRunTime;

// Button States
enum ButtonState { NONE, SHORT_PRESS, LONG_PRESS };

// Function Prototypes
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
void drawrgb();
void drawOSC();

// --- Button Handling Logic ---
ButtonState checkButton() {
  static unsigned long pressTime = 0;
  static bool isPressed = false;
  static bool longPressHandled = false;
  
  // INPUT_PULLUP means LOW is pressed
  bool currentState = digitalRead(BUTTON_PIN) == LOW; 
  
  if (currentState && !isPressed) {
    pressTime = millis();
    isPressed = true;
    longPressHandled = false;
  } else if (currentState && isPressed) {
    if (!longPressHandled && (millis() - pressTime > 600)) {
      longPressHandled = true;
      return LONG_PRESS; // Triggered while holding
    }
  } else if (!currentState && isPressed) {
    isPressed = false;
    if (!longPressHandled && (millis() - pressTime > 50)) { 
      return SHORT_PRESS; // Triggered on release
    }
  }
  return NONE;
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  EEPROM.begin(EEPROM_SIZE);

  tft.init(TFT_WIDTH, TFT_HEIGHT); 
  tft.setRotation(1); // Landscape mode
  tft.invertDisplay(true); // Adjust if colors look negative

  BLUE = tft.color565(50, 50, 255);

  tft.fillScreen(BLACK);
  tft.setTextSize(3);
  tft.setTextColor(WHITE);
  tft.setCursor(50, 10);
  tft.print("This Project");
  tft.setCursor(90, 50);
  tft.print("Done By");
  tft.setTextSize(2);
  tft.setTextColor(YELLOW);
  tft.setCursor(50, 110);
  tft.print("Teach Me Something");
  tft.setTextSize(3);
  tft.setCursor(5, 160);
  tft.setTextColor(GREEN);
  tft.print("Loading...");

  for (int i=0; i < 320; i+=2) {
    tft.fillRect(0, 210, i, 10, RED);
    delay(5);
  }

  drawHome();
}

void loop() {
  ButtonState btn = checkButton();

  switch (currentpage) {
    case 0: // Main Menu
      if (btn == SHORT_PRESS) {
        menuSelection = (menuSelection + 1) % 3; // Cycle 0, 1, 2
        drawHome();
      } else if (btn == LONG_PRESS) {
        if (menuSelection == 0) {
          currentpage = 1;
          startGame();
        } else if (menuSelection == 1) {
          currentpage = 2;
          drawrgb();
        } else if (menuSelection == 2) {
          currentpage = 3;
          drawOSC();
        }
      }
      break;

    case 1: // Flappy Bird
      if (btn == LONG_PRESS) {
        currentpage = 0; // Back to Menu
        drawHome();
      } else if (btn == SHORT_PRESS) {
        if (crashed) {
          startGame();
        } else if (!running) {
          tft.fillRect(0, 0, 320, 80, BLUE); // Clear start text
          running = true;
          fallRate = -8;
        } else {
          fallRate = -8; // Flap up
        }
      }

      if (millis() > nextDrawLoopRunTime && !crashed) {
        drawLoop();
        checkCollision();
        nextDrawLoopRunTime += DRAW_LOOP_INTERVAL;
      }
      break;

    case 2: // RGB Mixer
    case 3: // Oscilloscope
      if (btn == LONG_PRESS || btn == SHORT_PRESS) {
        currentpage = 0; // Back to Menu
        drawHome();
      }
      break;
  }
}

void drawHome() {
  tft.fillScreen(BLACK);
  tft.drawRoundRect(0, 0, 319, 240, 8, WHITE); // Border
  tft.setFont(); // Default font

  tft.setCursor(70, 20);
  tft.setTextSize(3);
  tft.setTextColor(WHITE);
  tft.print("MAIN MENU");

  String options[3] = {"1. Flappy Bird", "2. RGB Mixer", "3. Oscilloscope"};
  
  for(int i=0; i<3; i++) {
    if(menuSelection == i) {
       tft.fillRoundRect(30, 70 + i*50, 260, 40, 8, RED); // Highlighted
    } else {
       tft.fillRoundRect(30, 70 + i*50, 260, 40, 8, BLUE); // Normal
    }
    tft.drawRoundRect(30, 70 + i*50, 260, 40, 8, WHITE);
    
    tft.setCursor(50, 80 + i*50);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.print(options[i]);
  }

  tft.setCursor(20, 220);
  tft.setTextSize(1);
  tft.setTextColor(YELLOW);
  tft.print("Short Press: Next Item | Long Press: Select Item");
}

void startGame() {
  flX = 50;
  flY = 25;
  fallRate = 0; 
  pillarPos = 320;  
  gapPosition = 60; 
  crashed = false;  
  score = 0;       
  highScore = EEPROM.read(addr);    
  
  tft.setFont(&Org_01);              
  tft.fillScreen(BLUE);              
  tft.setTextColor(YELLOW);          
  tft.setTextSize(3);                
  tft.setCursor(5, 20);              
  tft.println("Flappy Bird");
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.println(" Press to flap!!");
  tft.setTextColor(GREEN);
  tft.setCursor(60, 60);
  tft.print("Top Score : ");
  tft.setTextColor(RED);
  tft.setTextSize(3);
  tft.print(highScore);

  // Draw Ground
  int ty = 230; int tx = 0;
  for ( tx = 0; tx <= 300; tx += 20) {
    tft.fillTriangle(tx, ty, tx + 10, ty, tx, ty + 10, GREEN);
    tft.fillTriangle(tx + 10, ty + 10, tx + 10, ty, tx, ty + 10, YELLOW);
    tft.fillTriangle(tx + 10, ty, tx + 20, ty, tx + 10, ty + 10, YELLOW);
    tft.fillTriangle(tx + 20, ty + 10, tx + 20, ty, tx + 10, ty + 10, GREEN);
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
    if (pillarPos == 0) {
      score = score + 5;
    }
    else if (pillarPos < -50) {
      pillarPos = 320;
      gapPosition = random(20, 120);    
    }
  }

  drawPillar(pillarPos, gapPosition);   
  drawFlappy(flX, flY);            
  switch (currentWing) {     
    case 0: case 1: drawWing1(flX, flY); break;  
    case 2: case 3: drawWing2(flX, flY); break;  
    case 4: case 5: drawWing3(flX, flY); break;  
  }
  
  if (score > 0 && score >= highScore) currentpcolour = YELLOW;
  else currentpcolour = GREEN;

  currentWing++;   
  if (currentWing == 6 ) currentWing = 0; 
}

void checkCollision() {
  if (flY > 206) crashed = true;

  if (flX + 34 > pillarPos && flX < pillarPos + 50) {
    if (flY < gapPosition || flY + 24 > gapPosition + 90) crashed = true;
  }

  if (crashed) {      
    tft.setTextColor(RED);   
    tft.setTextSize(5);      
    tft.setCursor(20, 75);   
    tft.print("Game Over!"); 

    tft.setTextSize(4);      
    tft.setCursor(75, 125);  
    tft.print("Score:");     

    tft.setCursor(220, 125); 
    tft.setTextSize(5);      
    tft.setTextColor(WHITE);
    tft.print(score);        

    if (score > highScore) {           
      highScore = score;
      EEPROM.write(addr, highScore);  
      EEPROM.commit(); // ESP32 requires commit to save
      tft.setCursor(75, 175);          
      tft.setTextSize(4);              
      tft.setTextColor(YELLOW);
      tft.print("NEW HIGH!");          
    }
    running = false;      
  }
}

// ---- Drawing Functions ----
void drawPillar(int x, int gap) {  
  tft.fillRect(x + 2, 2, 46, gap - 4, currentpcolour);
  tft.fillRect(x + 2, gap + 92, 46, 136 - gap, currentpcolour);
  tft.drawRect(x, 0, 50, gap, BLACK);
  tft.drawRect(x + 1, 1, 48, gap - 2, BLACK);
  tft.drawRect(x, gap + 90, 50, 140 - gap, BLACK);
  tft.drawRect(x + 1, gap + 91 , 48, 138 - gap, BLACK);
}

void clearPillar(int x, int gap) {  
  tft.fillRect(x + 45, 0, 5, gap, BLUE);
  tft.fillRect(x + 45, gap + 90, 5, 140 - gap, BLUE);
}

void clearFlappy(int x, int y) {  
  tft.fillRect(x, y, 34, 24, BLUE);
}

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

// Static apps since there is no touch
void drawrgb() {
  tft.setFont();
  tft.fillScreen(BLACK);
  tft.setCursor(90, 20);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.print("COLOUR : ");

  tft.drawRect(BAR_MINY, BLUEBAR_MINX, BAR_HEIGHT, BAR_WIDTH, WHITE); 
  tft.drawRect(BAR_MINY, GREENBAR_MINX, BAR_HEIGHT, BAR_WIDTH, WHITE); 
  tft.drawRect(BAR_MINY, REDBAR_MINX, BAR_HEIGHT, BAR_WIDTH, WHITE); 
  tft.fillRect(BAR_MINY + 12, REDBAR_MINX + 3, BAR_WIDTH - 10, BAR_WIDTH - 5, RED);
  tft.fillRect(BAR_MINY + 12, GREENBAR_MINX + 3, BAR_WIDTH - 10, BAR_WIDTH - 5, GREEN);
  tft.fillRect(BAR_MINY + 12, BLUEBAR_MINX + 3, BAR_WIDTH - 10, BAR_WIDTH - 5, BLUE);

  tft.setCursor(20, 220);
  tft.setTextSize(1);
  tft.print("Press button to exit");
}

void drawOSC() {
  tft.setFont();
  tft.fillScreen(BLACK);
  tft.setCursor(60, 100);
  tft.setTextSize(2);
  tft.setTextColor(GREEN);
  tft.print("Oscilloscope");
  tft.setCursor(20, 140);
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.print("Requires touch to operate.");
  
  tft.setCursor(20, 220);
  tft.print("Press button to exit");
}