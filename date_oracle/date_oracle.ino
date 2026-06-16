// Date-Night Decision Oracle -- Heltec WiFi LoRa 32 V2
// Uses only the onboard OLED and the onboard PRG button (GPIO 0). No wiring.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "date_ideas_data.h"

#define OLED_SDA     4
#define OLED_SCL     15
#define OLED_RST     16
#define OLED_VEXT    21    // active-LOW; powers OLED on LoRa V2
#define FEEDBACK_LED 25
#define BUTTON_PIN   0     // onboard PRG button (active LOW)

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDR     0x3C
#define LONG_PRESS_MS 700
#define IDLE_MS       30000

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define MAX_CATEGORIES    8
#define MAX_ITEMS_PER_CAT 32

struct Category {
  const char* name;
  const char* items[MAX_ITEMS_PER_CAT];
  uint8_t     count;
};

Category cats[MAX_CATEGORIES];
uint8_t  NUM_CATS = 0;

static char ideasRam[IDEAS_BLOB_CAPACITY];

uint8_t  curCat = 0;
int8_t   lastPick[MAX_CATEGORIES];
uint32_t lastAct = 0;
bool     resultShown = false;

void parseIdeas() {
  memcpy(ideasRam, IDEAS.data, IDEAS_BLOB_CAPACITY);
  ideasRam[IDEAS_BLOB_CAPACITY - 1] = 0;

  int8_t idx = -1;
  char* p = ideasRam;
  while (*p) {
    while (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t') p++;
    if (!*p) break;

    char* line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    if (*p) { *p = 0; p++; }

    char* end = line + strlen(line);
    while (end > line && (end[-1] == ' ' || end[-1] == '\t')) { end--; *end = 0; }
    if (!*line) continue;

    if (*line == '[') {
      if (idx + 1 >= MAX_CATEGORIES) break;
      idx++;
      char* name = line + 1;
      char* close = strchr(name, ']');
      if (close) *close = 0;
      cats[idx].name = name;
      cats[idx].count = 0;
    } else if (idx >= 0 && cats[idx].count < MAX_ITEMS_PER_CAT) {
      cats[idx].items[cats[idx].count++] = line;
    }
  }
  NUM_CATS = idx + 1;

  if (NUM_CATS == 0) {
    cats[0].name = "EMPTY";
    cats[0].items[0] = "edit date_ideas.txt";
    cats[0].items[1] = "or use the web app";
    cats[0].count = 2;
    NUM_CATS = 1;
  }
}

void drawHeart(int16_t cx, int16_t cy, uint8_t r) {
  display.fillCircle(cx - r, cy, r, SSD1306_WHITE);
  display.fillCircle(cx + r, cy, r, SSD1306_WHITE);
  display.fillTriangle(cx - 2*r, cy, cx + 2*r, cy, cx, cy + 2*r + r/2, SSD1306_WHITE);
}

void drawCentered(const char* t, uint8_t size, int16_t y) {
  display.setTextSize(size);
  int16_t lw = (int16_t)strlen(t) * 6 * size;
  int16_t x = (SCREEN_WIDTH - lw) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, y);
  display.print(t);
}

void drawWrappedCentered(const char* text, uint8_t size, int16_t top) {
  display.setTextSize(size);
  uint8_t charW = 6 * size, lineH = 8 * size;
  uint8_t maxChars = SCREEN_WIDTH / charW;
  if (maxChars > 23) maxChars = 23;

  char buf[64];
  strncpy(buf, text, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = 0;

  char lines[4][24];
  uint8_t n = 0;
  lines[0][0] = 0;

  char* tok = strtok(buf, " ");
  while (tok && n < 4) {
    if (lines[n][0] == 0) {
      strncpy(lines[n], tok, 23); lines[n][23] = 0;
    } else if (strlen(lines[n]) + 1 + strlen(tok) <= maxChars) {
      strcat(lines[n], " ");
      strcat(lines[n], tok);
    } else {
      n++;
      if (n >= 4) break;
      strncpy(lines[n], tok, 23); lines[n][23] = 0;
    }
    tok = strtok(NULL, " ");
  }
  uint8_t nLines = n + 1;
  if (nLines > 4) nLines = 4;

  int16_t blockH = nLines * lineH;
  int16_t y = top + ((SCREEN_HEIGHT - top) - blockH) / 2;
  if (y < top) y = top;

  for (uint8_t i = 0; i < nLines; i++) {
    int16_t lw = (int16_t)strlen(lines[i]) * charW;
    int16_t x = (SCREEN_WIDTH - lw) / 2;
    if (x < 0) x = 0;
    display.setCursor(x, y);
    display.print(lines[i]);
    y += lineH;
  }
}

void scatterSparkles(uint8_t n) {
  for (uint8_t i = 0; i < n; i++) {
    display.drawPixel(random(SCREEN_WIDTH), 12 + random(SCREEN_HEIGHT - 12), SSD1306_WHITE);
  }
}

void drawFrameHearts(uint8_t r) {
  drawHeart(4 + r, SCREEN_HEIGHT - 2 - r, r);
  drawHeart(SCREEN_WIDTH - 4 - r, SCREEN_HEIGHT - 2 - r, r);
}

void showShuffle(const char* category, const char* pick) {
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  drawCentered(category, 1, 2);
  display.setTextColor(SSD1306_WHITE);
  uint8_t size = strlen(pick) <= 12 ? 2 : 1;
  drawWrappedCentered(pick, size, 14);
  scatterSparkles(6);
  display.display();
}

void showResult(const char* category, const char* pick) {
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  drawCentered(category, 1, 2);
  display.setTextColor(SSD1306_WHITE);
  uint8_t size = strlen(pick) <= 12 ? 2 : 1;
  drawWrappedCentered(pick, size, 14);
  drawFrameHearts(3);
  display.display();
}

void renderIdle() {
  static const int8_t BOB[8] = {0, 1, 2, 1, 0, -1, -2, -1};
  int8_t bob = BOB[(millis() / 100) % 8];

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  drawHeart(12, 10 + bob, 3);
  drawHeart(SCREEN_WIDTH - 12, 10 + bob, 3);
  drawCentered(cats[curCat].name, 1, 4);
  drawCentered("TAP: pick", 2, 22);
  drawCentered("HOLD: next", 2, 44);
  display.display();
}

void showIdle() {
  renderIdle();
  resultShown = false;
}

void showBanner() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  drawCentered("MODE", 1, 14);
  uint8_t sz = strlen(cats[curCat].name) <= 10 ? 2 : 1;
  drawCentered(cats[curCat].name, sz, 34);
  display.display();
  delay(700);
}

void heartbeat() {
  digitalWrite(FEEDBACK_LED, HIGH); delay(70);
  digitalWrite(FEEDBACK_LED, LOW);  delay(90);
  digitalWrite(FEEDBACK_LED, HIGH); delay(110);
  digitalWrite(FEEDBACK_LED, LOW);  delay(260);
}

void revealBang(uint8_t times) {
  for (uint8_t i = 0; i < times; i++) {
    display.invertDisplay(true);
    digitalWrite(FEEDBACK_LED, HIGH);
    delay(95);
    display.invertDisplay(false);
    digitalWrite(FEEDBACK_LED, LOW);
    delay(95);
  }
}

void slotReveal(uint8_t cat) {
  Category& C = cats[cat];
  int8_t idx;
  do { idx = random(C.count); } while (C.count > 1 && idx == lastPick[cat]);
  lastPick[cat] = idx;

  uint16_t d = 18;
  for (uint8_t i = 0; i < 28; i++) {
    showShuffle(C.name, C.items[random(C.count)]);
    digitalWrite(FEEDBACK_LED, i & 1);
    delay(d);
    d += 2 + i / 3;
  }
  showResult(C.name, C.items[idx]);
  revealBang(3);
  heartbeat();
  heartbeat();
  resultShown = true;
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(FEEDBACK_LED, OUTPUT);
  parseIdeas();
  for (uint8_t i = 0; i < MAX_CATEGORIES; i++) lastPick[i] = -1;

  pinMode(OLED_VEXT, OUTPUT);
  digitalWrite(OLED_VEXT, LOW);
  delay(50);
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);  delay(20);
  digitalWrite(OLED_RST, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  randomSeed(esp_random());

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    for (;;);
  }
  display.setTextWrap(false);

  display.clearDisplay();
  drawHeart(28, 36, 6);
  drawHeart(SCREEN_WIDTH - 28, 36, 6);
  drawCentered("DATE-NIGHT", 1, 14);
  drawCentered("ORACLE", 2, 30);
  display.display();
  delay(1200);

  showIdle();
  lastAct = millis();
}

void loop() {
  static bool wasDown = false;
  static uint32_t downAt = 0;
  static uint32_t lastFrame = 0;

  bool down = (digitalRead(BUTTON_PIN) == LOW);

  if (down && !wasDown) { downAt = millis(); wasDown = true; }

  if (!down && wasDown) {
    uint32_t held = millis() - downAt;
    wasDown = false;
    lastAct = millis();
    if (held >= LONG_PRESS_MS) {
      curCat = (curCat + 1) % NUM_CATS;
      showBanner();
      showIdle();
    } else if (held > 25) {
      slotReveal(curCat);
    }
  }

  if (resultShown && (millis() - lastAct > IDLE_MS)) showIdle();

  if (!resultShown && millis() - lastFrame >= 100) {
    lastFrame = millis();
    renderIdle();
  }
}
