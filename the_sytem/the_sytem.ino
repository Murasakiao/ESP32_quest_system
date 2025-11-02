#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <math.h>

Preferences prefs;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUTTON_PIN 4

// Leveling system variables
int level = 1;
int xp = 0;
int xpToNext = 50;
int xpReward = 10;
int totalQuests = 0;

// Button handling
unsigned long lastPress = 0;
unsigned long pressStart = 0;
bool buttonPressed = false;
const int debounceDelay = 50;
const int holdTime = 1000; // 1 second hold = long press

bool questActive = false;

enum Screen { SCREEN_MAIN, SCREEN_STATUS, SCREEN_QUEST };
Screen currentScreen = SCREEN_MAIN; // Start on the main screen

// message notification
unsigned long lastMessageChange = 0;
const unsigned long messageInterval = 10000; // 10 seconds
String currentMainMessage = "";

// ---------------- MATRIX RAIN VARIABLES ----------------
#define DIGIT_W 3
#define DIGIT_H 5
#define X_SPACING 9
#define Y_SPACING 9
#define NUM_COLS (SCREEN_WIDTH / X_SPACING)
#define MAX_TRAIL 16

int headY[NUM_COLS];
uint8_t colTrailLen[NUM_COLS];
uint8_t colBuf[NUM_COLS][MAX_TRAIL];
uint8_t colSpeedFrames[NUM_COLS];
uint8_t colFrameCounter[NUM_COLS];

// tiny 3x5 digits
const uint8_t tinyDigits[10][DIGIT_H] = {
  {0b111,0b101,0b101,0b101,0b111}, //0
  {0b010,0b110,0b010,0b010,0b111}, //1
  {0b111,0b001,0b111,0b100,0b111}, //2
  {0b111,0b001,0b111,0b001,0b111}, //3
  {0b101,0b101,0b111,0b001,0b001}, //4
  {0b111,0b100,0b111,0b001,0b111}, //5
  {0b111,0b100,0b111,0b101,0b111}, //6
  {0b111,0b001,0b010,0b010,0b010}, //7
  {0b111,0b101,0b111,0b101,0b111}, //8
  {0b111,0b101,0b111,0b001,0b111}  //9
};

// ---------------- BASIC FUNCTIONS ----------------
void drawTinyDigit(int x, int y, uint8_t n) {
  if (n > 9) return;
  for (int r = 0; r < DIGIT_H; r++) {
    uint8_t row = tinyDigits[n][r];
    for (int c = 0; c < DIGIT_W; c++) {
      if (row & (1 << (DIGIT_W - 1 - c))) display.drawPixel(x + c, y + r, SSD1306_WHITE);
    }
  }
}

void setupColumn(int col) {
  headY[col] = random(-8, 0);
  colTrailLen[col] = random(1, 5);
  colSpeedFrames[col] = random(2, 5);
  colFrameCounter[col] = 0;
  for (int t = 0; t < MAX_TRAIL; t++) colBuf[col][t] = random(10);
}

void updateMatrix() {
  for (int c = 0; c < NUM_COLS; c++) {
    colFrameCounter[c]++;
    if (colFrameCounter[c] >= colSpeedFrames[c]) {
      colFrameCounter[c] = 0;
      headY[c] += 1;

      for (int t = MAX_TRAIL - 1; t > 0; t--) colBuf[c][t] = colBuf[c][t - 1];

      uint8_t newDigit;
      do {
        newDigit = random(10);
      } while (newDigit == colBuf[c][1] && random(10) < 8);
      colBuf[c][0] = newDigit;

      if ((headY[c] - (int)colTrailLen[c]) * Y_SPACING > SCREEN_HEIGHT) {
        setupColumn(c);
      }
    }
  }

  display.clearDisplay();

  for (int c = 0; c < NUM_COLS; c++) {
    int x = c * X_SPACING;
    for (int t = 0; t < colTrailLen[c]; t++) {
      int rowIndex = headY[c] - t;
      int yPixel = rowIndex * Y_SPACING;
      if (yPixel < -DIGIT_H || yPixel > SCREEN_HEIGHT) continue;

      bool drawThis = false;
      if (t == 0 || t == 1) drawThis = true;
      else if (t == 2) drawThis = (random(3) == 0);
      else if (t == 3) drawThis = (random(4) == 0);
      else drawThis = (random(6) == 0);

      if (!drawThis) continue;
      drawTinyDigit(x, yPixel, colBuf[c][t]);
    }
  }
}

// ---------------- Rotating Cube ----------------
void drawDiamond3D(int cx, int cy, int half, int depth, float angle) {
  float fx_f[4] = {-half,  half,  half, -half};
  float fy_f[4] = {-half, -half,  half,  half};
  int fx[4], fy[4], bx[4], by[4];

  for (int i = 0; i < 4; i++) {
    float xNew = fx_f[i] * cos(angle) - fy_f[i] * sin(angle);
    float yNew = fx_f[i] * sin(angle) + fy_f[i] * cos(angle);
    fx[i] = round(xNew);
    fy[i] = round(yNew);
  }

  for (int i = 0; i < 4; i++) {
    bx[i] = fx[i];
    by[i] = fy[i] + depth;
  }

  for (int i = 0; i < 4; i++) {
    int j = (i + 1) % 4;
    display.drawLine(cx + fx[i], cy + fy[i], cx + fx[j], cy + fy[j], SSD1306_WHITE);
    display.drawLine(cx + bx[i], cy + by[i], cx + bx[j], cy + by[j], SSD1306_WHITE);
  }

  for (int i = 0; i < 4; i++) {
    display.drawLine(cx + fx[i], cy + fy[i], cx + bx[i], cy + by[i], SSD1306_WHITE);
  }
}

void showCubeAnimationUntilButton() {
  int cx = SCREEN_WIDTH / 2;
  float angle = 0;
  int half = 8, depth = 6;
  float y = SCREEN_HEIGHT / 2;
  bool goingUp = true;
  float speed = 0;
  unsigned long lastButtonCheck = millis();

  while (true) {
    // Vertical bounce
    if (goingUp) { speed -= 0.4; if (speed < -2.5) speed = -2.5; }
    else { speed += 0.2; if (speed > 1.5) speed = 1.5; }
    y += speed;
    if (y < 20) goingUp = false;
    else if (y > 30) goingUp = true;

    // Rotation
    angle += 0.1;
    if (angle > 2 * M_PI) angle -= 2 * M_PI;

    display.clearDisplay();
    drawDiamond3D(cx, (int)y, half, depth, angle);
    display.setTextSize(1);
    display.setCursor(15, 52);
    display.print("[ CLAIM REWARD ]");
    display.display();
    delay(25);

    if (millis() - lastButtonCheck > 100) {
      if (digitalRead(BUTTON_PIN) == LOW) {
        while (digitalRead(BUTTON_PIN) == LOW);
        break;
      }
      lastButtonCheck = millis();
    }
  }

  display.clearDisplay();
  display.display();
  delay(200);
}

// ---------------- PLAYER DATA ----------------
void savePlayerData() {
  prefs.putInt("level", level);
  prefs.putInt("xp", xp);
  prefs.putInt("xpToNext", xpToNext);
  prefs.putInt("screen", (int)currentScreen);
  prefs.putInt("totalQuests", totalQuests);
}

// ---------------- SCREEN FUNCTIONS ----------------
void showXPAdded() {
  const int textSize = 2;
  const int baseX = 30;
  const int baseY = 25;
  const int frames = 20;
  const int frameDelay = 40;
  
  for (int i = 0; i < frames; i++) {
    display.clearDisplay();
    display.setTextSize(textSize);
    display.setTextColor(SSD1306_WHITE);
    
    // Main text
    display.setCursor(baseX, baseY);
    display.print("+");
    display.print(xpReward);
    display.println("XP!");
    
    // OPTION 1: Horizontal scan line flicker (glitch effect)
    if (random(100) < 40) { // 40% chance per frame
      int scanLineY = baseY + random(0, 20);
      int flickerWidth = random(40, 90);
      int flickerX = baseX + random(-5, 15);
      display.drawFastHLine(flickerX, scanLineY, flickerWidth, SSD1306_BLACK);
      delay(2); // Very brief
      display.drawFastHLine(flickerX, scanLineY, flickerWidth, SSD1306_WHITE);
    }
    
    // OPTION 2: Sideways displacement with trailing
    if (random(100) < 35) { // 35% chance per frame
      int shiftAmount = random(2, 6);
      int direction = random(2); // 0 = left, 1 = right
      
      for (int offset = 0; offset < shiftAmount; offset++) {
        int xOffset = (direction == 0) ? -offset : offset;
        display.setCursor(baseX + xOffset, baseY);
        display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
        display.print("+");
        display.print(xpReward);
        display.println("XP!");
      }
    }
    
    // OPTION 3: Vertical scan artifact (V-shaped glitch)
    if (random(100) < 30) { // 30% chance per frame
      int startX = baseX + random(0, 60);
      int numLines = random(2, 4);
      for (int v = 0; v < numLines; v++) {
        int vLine = baseY + random(0, 20);
        display.drawFastVLine(startX + v * 3, vLine, 8, SSD1306_WHITE);
      }
    }
    
    display.display();
    delay(frameDelay);
  }


  // hold visible for a moment before transitioning out
  delay(600);

  // XP progress bar animation
  int barX = 35;
  int barY = 50;
  int barW = 60;
  int barH = 3;
  float progress = (float)xp / xpToNext;

  for (int i = 0; i <= (int)(progress * barW); i += 2) {
    display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
    display.fillRect(barX, barY, i, barH, SSD1306_WHITE);
    display.display();
    delay(10);
  }

  delay(800);
  savePlayerData();
  showStatus();
}

void showLevelUp() {
  // Flash and bounce "LEVEL UP!"
  for (int i = 0; i < 3; i++) {
    display.clearDisplay();
    display.display();
    delay(100);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 20);
    display.println("LEVEL UP!");
    display.display();
    delay(100);
  }

  // Bounce animation
  for (int y = 20; y > 10; y--) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(15, y);
    display.println("LEVEL UP!");
    display.display();
    delay(15);
  }

  delay(1500);

  // Show new level in large text
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(40, 15);
  display.println("NEW LEVEL");
  display.setTextSize(3);
  display.setCursor(55, 30);
  display.println(level);
  display.display();

  delay(3000);
  savePlayerData();
  showStatus();
}

void showQuest() {
  updateMatrix();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(17, 30);
  display.print("[ ENTER QUEST ]");
  display.display();
}

void showMain() {
  updateMatrix();

  if (millis() - lastMessageChange > messageInterval) {
    const char* messages[] = {
      "  - STAY FOCUSED -  ",
      "  - YOU GOT THIS! -  ",
      " - NO NEW QUESTS - ",
      "- NO NOTIFICATION -"
    };
    currentMainMessage = messages[random(4)];
    lastMessageChange = millis();
  }

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 30);
  display.print(currentMainMessage);
  display.display();
}

void showStatus() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 10);
  display.print("PLAYER: JULIUS");
  display.setCursor(5, 20);
  display.print("Level: ");
  display.println(level);
  display.setCursor(5, 30);
  display.print("XP: ");
  display.print(xp);
  display.print("/");
  display.println(xpToNext);
  // XP progress bar
  int barX = 5;
  int barY = 40;
  int barW = 60;
  int barH = 3;
  float progress = (float)xp / xpToNext;

  // for (int i = 0; i <= (int)(progress * barW); i += 2) {
  display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
  display.fillRect(barX, barY, progress * barW, barH, SSD1306_WHITE);
  display.display();

  display.setCursor(5, 45 );
  display.print("QUESTS: ");
  display.print(totalQuests);
  display.display();
}

void fadeOut() {
  for (int y = 0; y < SCREEN_HEIGHT; y += 2) {
    display.drawFastHLine(0, y, SCREEN_WIDTH, SSD1306_BLACK);
    display.display();
    delay(15);
  }
}

// ---------------- QUEST SYSTEM ----------------
void startQuest() {
  questActive = true;
  const char spinnerFrames[] = {'|', '/', '-', '\\'};
  const int frameCount = sizeof(spinnerFrames);

  unsigned long animationStart = millis();
  unsigned long animationDuration = 2500;

  while (millis() - animationStart < animationDuration) {
    for (int i = 0; i < frameCount; i++) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(15, 20);
      display.println("Starting Quest...");
      display.setTextSize(2);
      display.setCursor(55, 35);
      display.write(spinnerFrames[i]);
      display.display();
      delay(150);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 25);
  display.println("> Quest Started! <");
  display.display();
  delay(700);

  unsigned long questDuration = 120000;
  unsigned long startTime = millis();

  while (millis() - startTime < questDuration) {
    unsigned long timeLeft = (questDuration - (millis() - startTime)) / 1000;
    int hours = timeLeft / 3600;
    int minutes = (timeLeft % 3600) / 60;
    int seconds = timeLeft % 60;
    float progress = (float)(millis() - startTime) / questDuration;

    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(7, 10);
    display.println(" QUEST IN PROGRESS");

    display.setTextSize(2);
    display.setCursor(16, 35);
    display.print(timeStr);
    display.fillRect(12, 62, progress * 100, 2, SSD1306_WHITE);
    display.display();
    
    // Check button every small step (responsive)
    unsigned long frameStart = millis();
    while (millis() - frameStart < 200) {
      if (digitalRead(BUTTON_PIN) == LOW) {
        // Wait for release to prevent double detection
        while (digitalRead(BUTTON_PIN) == LOW);
        
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(20, 35);
        display.print("QUEST CANCELLED");
        display.display();
        delay(1000);
        questActive = false;
        return;  // Exit immediately
      }
    }

  }

  Serial.println("Quest complete!");
  showCubeAnimationUntilButton(); // 🔹 Cube animation before XP

  xpReward = random(10, 15) + (level * 1.5);
  xp += xpReward;
  totalQuests += 1;
  Serial.print("Gained ");
  Serial.print(xpReward);
  Serial.println(" XP");

  if (xp >= xpToNext) {
    xp -= xpToNext;
    level++;
    xpToNext = (int)(xpToNext * 1.5);
    showLevelUp();
  } else {
    showXPAdded();
  }

  questActive = false;
}

// ---------------- BUTTON & SCREEN LOGIC ----------------
void handleShortPress() {
  if (questActive) return;

  fadeOut();

  if (currentScreen == SCREEN_MAIN) {
    currentScreen = SCREEN_STATUS;
    showStatus();
  } else if (currentScreen == SCREEN_STATUS) {
    currentScreen = SCREEN_QUEST;
    showQuest();
  } else if (currentScreen == SCREEN_QUEST) {
    currentScreen = SCREEN_MAIN;
    showMain();
  }

}

void handleLongPress() {
  if (questActive) return;
  if (currentScreen == SCREEN_QUEST) startQuest();
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  prefs.begin("player", false);
  level = prefs.getInt("level", 1);
  xp = prefs.getInt("xp", 0);
  xpToNext = prefs.getInt("xpToNext", 50);
  currentScreen = (Screen)prefs.getInt("screen", 0);

  totalQuests = prefs.getInt("totalQuests", 0);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  randomSeed(esp_random());
  for (int c = 0; c < NUM_COLS; c++) setupColumn(c);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 30);
  display.println("WELCOME PLAYER!");
  display.display();
  delay(1000);
  showStatus();
}

// ---------------- LOOP ----------------
void loop() {
  int buttonState = digitalRead(BUTTON_PIN);

  // Detect button press start
  if (buttonState == LOW && !buttonPressed && millis() - lastPress > debounceDelay) {
    buttonPressed = true;
    pressStart = millis();
  }

  // Detect button release
  if (buttonState == HIGH && buttonPressed) {
    unsigned long pressDuration = millis() - pressStart;
    buttonPressed = false;
    lastPress = millis();

    if (pressDuration < holdTime) handleShortPress();
    else handleLongPress();
  }

  // Animate matrix rain when on main or quest screen
  if (!questActive && currentScreen == 0) {
    showMain();
  } else if (!questActive && currentScreen == 2) {
    showQuest();
  }

  delay(20);
}
