#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Preferences.h>

//==================================================
// TELEMETRY CONFIGURATION
//==================================================
const char *WIFI_SSID = "YOUR_WIFI_NAME";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char *SERVER_URL = "http://192.168.128.66:4000/api/telemetry";

String playerName = "Player 1";
String branchName = "Computer";
unsigned long lastPostMs = 0;
const unsigned long POST_INTERVAL_MS = 2000;

//==================================================
// HARDWARE
//==================================================
#define TFT_CS 5
#define TFT_DC 15
#define TFT_RST 21

#define LEFT_X 34
#define LEFT_Y 35
#define LEFT_SW 4

#define RIGHT_X 33
#define RIGHT_Y 25
#define RIGHT_SW 27

#define BUZZER_PIN 13
#define VIBRATION_PIN 14

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
Preferences prefs;

constexpr int SCREEN_W = 160;
constexpr int SCREEN_H = 128;

constexpr int JOYSTICK_LOW = 1200;
constexpr int JOYSTICK_HIGH = 2900;
constexpr int JOYSTICK_CENTER = 2048;

//==================================================
// SHARED APPLICATION STATE
//==================================================
enum AppState {
  GAME_MENU,
  SNAKE_PLAYING,
  SNAKE_GAME_OVER,
  TETRIS_PLAYING,
  TETRIS_GAME_OVER,
  DOOM_PLAYING,
  DOOM_GAME_OVER,
  RACER_PLAYING,
  RACER_GAME_OVER,
  PAC_PLAYING,
  PAC_GAME_OVER,
  SPACE_PLAYING,
  SPACE_GAME_OVER
};

struct InputFrame {
  int x;
  int y;
  int rightX;
  int rightY;
  bool left;
  bool right;
  bool up;
  bool down;
  bool turnLeft;
  bool turnRight;
  bool leftPressed;
  bool rightPressed;
  bool rightHeld;
};

AppState appState = GAME_MENU;
uint8_t menuSelection = 0;
bool menuDirectionLatched = false;
constexpr uint8_t GAME_COUNT = 6;
constexpr uint8_t MENU_VISIBLE_ROWS = 4;
uint8_t menuWindowStart = 0;

unsigned int snakeHighScore = 0;
unsigned int tetrisHighScore = 0;
unsigned int doomHighScore = 0;
unsigned int racerHighScore = 0;
unsigned int pacHighScore = 0;
unsigned int spaceHighScore = 0;

void enterMenu();
void resetSnake();
void updateSnake(const InputFrame &input);
void updateSnakeGameOver(const InputFrame &input);
void resetTetris();
void updateTetris(const InputFrame &input);
void updateTetrisGameOver(const InputFrame &input);
void resetDoom();
void updateDoom(const InputFrame &input);
void updateDoomGameOver(const InputFrame &input);
void resetRacer();
void updateRacer(const InputFrame &input);
void updateRacerGameOver(const InputFrame &input);
void resetPac();
void updatePac(const InputFrame &input);
void updatePacGameOver(const InputFrame &input);
void resetSpace();
void updateSpace(const InputFrame &input);
void updateSpaceGameOver(const InputFrame &input);

InputFrame readInput() {
  static bool previousLeftSwitch = HIGH;
  static bool previousRightSwitch = HIGH;

  InputFrame input{};
  input.x = analogRead(LEFT_X);
  input.y = analogRead(LEFT_Y);
  input.rightX = analogRead(RIGHT_X);
  input.rightY = analogRead(RIGHT_Y);

  const int xDeviation = abs(input.x - JOYSTICK_CENTER);
  const int yDeviation = abs(input.y - JOYSTICK_CENTER);

  if (yDeviation >= xDeviation) {
    input.left = input.y < JOYSTICK_LOW;
    input.right = input.y > JOYSTICK_HIGH;
  } else {
    input.up = input.x > JOYSTICK_HIGH;
    input.down = input.x < JOYSTICK_LOW;
  }

  input.turnLeft = input.rightY < JOYSTICK_LOW;
  input.turnRight = input.rightY > JOYSTICK_HIGH;

  const bool leftSwitch = digitalRead(LEFT_SW);
  const bool rightSwitch = digitalRead(RIGHT_SW);

  input.leftPressed = previousLeftSwitch == HIGH && leftSwitch == LOW;
  input.rightPressed = previousRightSwitch == HIGH && rightSwitch == LOW;
  input.rightHeld = rightSwitch == LOW;

  previousLeftSwitch = leftSwitch;
  previousRightSwitch = rightSwitch;
  return input;
}

void beepSelect() { tone(BUZZER_PIN, 900, 45); }

void beepEat() { tone(BUZZER_PIN, 1200, 60); }

void beepGameOver() { tone(BUZZER_PIN, 240, 280); }

unsigned long vibrationUntil = 0;

void vibrateFor(unsigned long durationMs) {
  digitalWrite(VIBRATION_PIN, HIGH);

  const unsigned long requestedUntil = millis() + durationMs;
  if (vibrationUntil == 0 ||
      static_cast<long>(requestedUntil - vibrationUntil) > 0) {
    vibrationUntil = requestedUntil;
  }
}

void serviceHaptics() {
  if (vibrationUntil != 0 &&
      static_cast<long>(millis() - vibrationUntil) >= 0) {
    digitalWrite(VIBRATION_PIN, LOW);
    vibrationUntil = 0;
  }
}

//==================================================
// MENU
//==================================================
const char *const GAME_LABELS[] = {
    "SNAKE", "TETRIS", "DOOM-LITE", "RACER", "PAC-MAZE", "SPACE RAID",
};

unsigned int menuHighScore(uint8_t index) {
  switch (index) {
  case 0:
    return snakeHighScore;
  case 1:
    return tetrisHighScore;
  case 2:
    return doomHighScore;
  case 3:
    return racerHighScore;
  case 4:
    return pacHighScore;
  case 5:
    return spaceHighScore;
  default:
    return 0;
  }
}

void updateMenuWindow() {
  if (menuSelection < menuWindowStart) {
    menuWindowStart = menuSelection;
  } else if (menuSelection >= menuWindowStart + MENU_VISIBLE_ROWS) {
    menuWindowStart = menuSelection - MENU_VISIBLE_ROWS + 1;
  }
}

void showMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(25, 5);
  tft.print("MINI ARCADE");

  tft.setTextSize(1);
  updateMenuWindow();
  for (uint8_t row = 0; row < MENU_VISIBLE_ROWS; ++row) {
    const uint8_t gameIndex = menuWindowStart + row;
    if (gameIndex >= GAME_COUNT) {
      break;
    }
    const bool selected = gameIndex == menuSelection;
    tft.setTextColor(selected ? ST77XX_YELLOW : ST77XX_WHITE);
    tft.setCursor(8, 31 + row * 16);
    tft.print(selected ? "> " : "  ");
    tft.print(GAME_LABELS[gameIndex]);
    tft.setCursor(100, 31 + row * 16);
    tft.print("HI ");
    tft.print(menuHighScore(gameIndex));
  }

  tft.drawFastHLine(12, 94, 136, ST77XX_BLUE);

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(22, 101);
  tft.print("UP/DOWN: SELECT");
  tft.setCursor(34, 115);
  tft.print("RIGHT: PLAY");
}

void enterMenu() {
  appState = GAME_MENU;
  menuDirectionLatched = true;
  showMenu();
}

void startSelectedGame() {
  beepSelect();
  switch (menuSelection) {
  case 0:
    resetSnake();
    break;
  case 1:
    resetTetris();
    break;
  case 2:
    resetDoom();
    break;
  case 3:
    resetRacer();
    break;
  case 4:
    resetPac();
    break;
  case 5:
    resetSpace();
    break;
  default:
    menuSelection = 0;
    resetSnake();
    break;
  }
}

void updateMenu(const InputFrame &input) {
  const bool menuDirection = input.up || input.down;

  if (!menuDirection) {
    menuDirectionLatched = false;
  } else if (!menuDirectionLatched) {
    if (input.down) {
      menuSelection = (menuSelection + 1) % GAME_COUNT;
    } else {
      menuSelection = (menuSelection + GAME_COUNT - 1) % GAME_COUNT;
    }
    menuDirectionLatched = true;
    beepSelect();
    showMenu();
  }

  if (input.rightPressed) {
    startSelectedGame();
  }
}

//==================================================
// SNAKE
//==================================================
constexpr int SNAKE_TOP_BAR = 10;
constexpr int SNAKE_CELL = 8;
constexpr int SNAKE_GRID_W = SCREEN_W / SNAKE_CELL;
constexpr int SNAKE_GRID_H = (SCREEN_H - SNAKE_TOP_BAR) / SNAKE_CELL;
constexpr int SNAKE_MAX_LENGTH = SNAKE_GRID_W * SNAKE_GRID_H;
constexpr int SNAKE_BOTTOM_MARGIN =
    SCREEN_H - (SNAKE_TOP_BAR + SNAKE_GRID_H * SNAKE_CELL);

constexpr unsigned long SNAKE_BASE_MOVE_TIME = 140;
constexpr unsigned long SNAKE_MIN_MOVE_TIME = 60;
constexpr int SNAKE_SPEEDUP_PER_POINT = 3;
constexpr int SNAKE_WRAP_SCORE = 29;

static_assert(SNAKE_WRAP_SCORE == 29,
              "Horizontal wrapping must begin after crossing 28 points");

struct Point {
  int x;
  int y;
};

bool snakeResolveBoundary(Point &head);

enum Direction { UP, DOWN, LEFT, RIGHT };

Point snakeSegments[SNAKE_MAX_LENGTH];
Point snakeFood;
int snakeLength = 0;
Direction snakeDirection = RIGHT;
Direction snakePendingDirection = RIGHT;
int snakeScore = 0;
bool snakePaused = false;
unsigned long snakeLastMove = 0;
unsigned long snakeWrapNoticeUntil = 0;

unsigned long snakeMoveInterval() {
  long interval = static_cast<long>(SNAKE_BASE_MOVE_TIME) -
                  static_cast<long>(snakeScore * SNAKE_SPEEDUP_PER_POINT);

  if (interval < static_cast<long>(SNAKE_MIN_MOVE_TIME)) {
    interval = SNAKE_MIN_MOVE_TIME;
  }

  return static_cast<unsigned long>(interval);
}

void drawSnakeCell(int x, int y, uint16_t color) {
  tft.fillRect(x * SNAKE_CELL, SNAKE_TOP_BAR + y * SNAKE_CELL, SNAKE_CELL - 1,
               SNAKE_CELL - 1, color);
}

void drawSnakeBottomBorder() {
  if (SNAKE_BOTTOM_MARGIN <= 0) {
    return;
  }

  const int borderY = SCREEN_H - SNAKE_BOTTOM_MARGIN;
  tft.drawFastHLine(0, borderY, SCREEN_W, ST77XX_WHITE);
}

void drawSnakeHud() {
  tft.fillRect(0, 0, SCREEN_W, SNAKE_TOP_BAR, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(2, 1);
  tft.print("Score:");
  tft.print(snakeScore);

  if (snakePaused) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(105, 1);
    tft.print("PAUSE");
  } else if (millis() < snakeWrapNoticeUntil) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(105, 1);
    tft.print("WRAP ON");
  }
}

bool spawnSnakeFood() {
  if (snakeLength >= SNAKE_MAX_LENGTH) {
    return false;
  }

  for (int attempt = 0; attempt < SNAKE_MAX_LENGTH * 2; ++attempt) {
    const Point candidate{random(SNAKE_GRID_W), random(SNAKE_GRID_H)};
    bool occupied = false;

    for (int i = 0; i < snakeLength; ++i) {
      if (snakeSegments[i].x == candidate.x &&
          snakeSegments[i].y == candidate.y) {
        occupied = true;
        break;
      }
    }

    if (!occupied) {
      snakeFood = candidate;
      drawSnakeCell(snakeFood.x, snakeFood.y, ST77XX_RED);
      return true;
    }
  }

  for (int y = 0; y < SNAKE_GRID_H; ++y) {
    for (int x = 0; x < SNAKE_GRID_W; ++x) {
      bool occupied = false;

      for (int i = 0; i < snakeLength; ++i) {
        if (snakeSegments[i].x == x && snakeSegments[i].y == y) {
          occupied = true;
          break;
        }
      }

      if (!occupied) {
        snakeFood = {x, y};
        drawSnakeCell(snakeFood.x, snakeFood.y, ST77XX_RED);
        return true;
      }
    }
  }

  return false;
}

bool snakeResolveBoundary(Point &head) {
  if (head.y < 0 || head.y >= SNAKE_GRID_H) {
    return false;
  }

  if (head.x >= 0 && head.x < SNAKE_GRID_W) {
    return true;
  }

  if (snakeScore < SNAKE_WRAP_SCORE) {
    return false;
  }

  head.x = head.x < 0 ? SNAKE_GRID_W - 1 : 0;
  return true;
}

void drawSnakeGameOver() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(25, 16);
  tft.print("GAME OVER");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(46, 52);
  tft.print("Score: ");
  tft.print(snakeScore);

  tft.setCursor(34, 68);
  tft.print("Snake High: ");
  tft.print(snakeHighScore);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(35, 92);
  tft.print("RIGHT: RESTART");
  tft.setCursor(47, 106);
  tft.print("LEFT: MENU");
}

void endSnakeGame() {
  vibrateFor(300);

  if (snakeScore > static_cast<int>(snakeHighScore)) {
    snakeHighScore = static_cast<unsigned int>(snakeScore);
    prefs.putUInt("high", snakeHighScore);
  }

  appState = SNAKE_GAME_OVER;
  beepGameOver();
  drawSnakeGameOver();
}

void resetSnake() {
  tft.fillScreen(ST77XX_BLACK);

  snakeLength = 4;
  snakeSegments[0] = {8, 6};
  snakeSegments[1] = {7, 6};
  snakeSegments[2] = {6, 6};
  snakeSegments[3] = {5, 6};

  snakeDirection = RIGHT;
  snakePendingDirection = RIGHT;
  snakeScore = 0;
  snakePaused = false;
  snakeWrapNoticeUntil = 0;
  snakeLastMove = millis();

  drawSnakeHud();
  drawSnakeBottomBorder();

  for (int i = 0; i < snakeLength; ++i) {
    drawSnakeCell(snakeSegments[i].x, snakeSegments[i].y, ST77XX_GREEN);
  }

  spawnSnakeFood();
  appState = SNAKE_PLAYING;
}

void updateSnakeDirection(const InputFrame &input) {
  Direction candidate = snakePendingDirection;

  if (input.left) {
    candidate = LEFT;
  } else if (input.right) {
    candidate = RIGHT;
  } else if (input.up) {
    candidate = UP;
  } else if (input.down) {
    candidate = DOWN;
  }

  const bool reversal = (candidate == LEFT && snakeDirection == RIGHT) ||
                        (candidate == RIGHT && snakeDirection == LEFT) ||
                        (candidate == UP && snakeDirection == DOWN) ||
                        (candidate == DOWN && snakeDirection == UP);

  if (!reversal) {
    snakePendingDirection = candidate;
  }
}

void moveSnake() {
  snakeDirection = snakePendingDirection;
  Point newHead = snakeSegments[0];

  switch (snakeDirection) {
  case UP:
    --newHead.y;
    break;
  case DOWN:
    ++newHead.y;
    break;
  case LEFT:
    --newHead.x;
    break;
  case RIGHT:
    ++newHead.x;
    break;
  }

  if (!snakeResolveBoundary(newHead)) {
    endSnakeGame();
    return;
  }

  const bool grow = newHead.x == snakeFood.x && newHead.y == snakeFood.y;
  const int collisionLimit = grow ? snakeLength : snakeLength - 1;

  for (int i = 0; i < collisionLimit; ++i) {
    if (snakeSegments[i].x == newHead.x && snakeSegments[i].y == newHead.y) {
      endSnakeGame();
      return;
    }
  }

  if (grow && snakeLength >= SNAKE_MAX_LENGTH) {
    endSnakeGame();
    return;
  }

  if (!grow) {
    const Point tail = snakeSegments[snakeLength - 1];
    drawSnakeCell(tail.x, tail.y, ST77XX_BLACK);
  }

  const int shiftStart = grow ? snakeLength : snakeLength - 1;
  for (int i = shiftStart; i > 0; --i) {
    snakeSegments[i] = snakeSegments[i - 1];
  }

  snakeSegments[0] = newHead;
  drawSnakeCell(newHead.x, newHead.y, ST77XX_GREEN);

  if (!grow) {
    return;
  }

  ++snakeLength;
  ++snakeScore;
  beepEat();
  vibrateFor(40);

  if (snakeScore == SNAKE_WRAP_SCORE) {
    snakeWrapNoticeUntil = millis() + 1500;
    tone(BUZZER_PIN, 1600, 80);
  }

  drawSnakeHud();

  if (!spawnSnakeFood()) {
    endSnakeGame();
  }
}

void updateSnake(const InputFrame &input) {
  if (input.leftPressed) {
    snakePaused = !snakePaused;
    if (!snakePaused) {
      snakeLastMove = millis();
    }
    drawSnakeHud();
  }

  if (snakePaused) {
    return;
  }

  updateSnakeDirection(input);

  const unsigned long now = millis();
  if (now - snakeLastMove >= snakeMoveInterval()) {
    snakeLastMove = now;
    moveSnake();
  }

  if (appState != SNAKE_PLAYING) {
    return;
  }

  if (snakeWrapNoticeUntil != 0 && now >= snakeWrapNoticeUntil) {
    snakeWrapNoticeUntil = 0;
    drawSnakeHud();
  }
}

void updateSnakeGameOver(const InputFrame &input) {
  if (input.rightPressed) {
    resetSnake();
  } else if (input.leftPressed) {
    enterMenu();
  }
}

//==================================================
// TETRIS CORE
//==================================================
constexpr int TETRIS_COLS = 10;
constexpr int TETRIS_ROWS = 18;
constexpr int TETRIS_CELL = 6;
constexpr int TETRIS_BOARD_X = 4;
constexpr int TETRIS_BOARD_Y = 9;
constexpr int TETRIS_PIECE_COUNT = 7;

#define TETRIS_MASK(row0, row1, row2, row3)                                    \
  (static_cast<uint16_t>(row0) | (static_cast<uint16_t>(row1) << 4) |          \
   (static_cast<uint16_t>(row2) << 8) | (static_cast<uint16_t>(row3) << 12))

// Piece order: I, O, T, S, Z, J, L.
const uint16_t TETRIS_MASKS[TETRIS_PIECE_COUNT][4] PROGMEM = {
    {
        TETRIS_MASK(0b0000, 0b1111, 0b0000, 0b0000),
        TETRIS_MASK(0b0010, 0b0010, 0b0010, 0b0010),
        TETRIS_MASK(0b0000, 0b0000, 0b1111, 0b0000),
        TETRIS_MASK(0b0100, 0b0100, 0b0100, 0b0100),
    },
    {
        TETRIS_MASK(0b0110, 0b0110, 0b0000, 0b0000),
        TETRIS_MASK(0b0110, 0b0110, 0b0000, 0b0000),
        TETRIS_MASK(0b0110, 0b0110, 0b0000, 0b0000),
        TETRIS_MASK(0b0110, 0b0110, 0b0000, 0b0000),
    },
    {
        TETRIS_MASK(0b0010, 0b0111, 0b0000, 0b0000),
        TETRIS_MASK(0b0010, 0b0110, 0b0010, 0b0000),
        TETRIS_MASK(0b0000, 0b0111, 0b0010, 0b0000),
        TETRIS_MASK(0b0010, 0b0011, 0b0010, 0b0000),
    },
    {
        TETRIS_MASK(0b0110, 0b0011, 0b0000, 0b0000),
        TETRIS_MASK(0b0010, 0b0110, 0b0100, 0b0000),
        TETRIS_MASK(0b0000, 0b0110, 0b0011, 0b0000),
        TETRIS_MASK(0b0001, 0b0011, 0b0010, 0b0000),
    },
    {
        TETRIS_MASK(0b0011, 0b0110, 0b0000, 0b0000),
        TETRIS_MASK(0b0100, 0b0110, 0b0010, 0b0000),
        TETRIS_MASK(0b0000, 0b0011, 0b0110, 0b0000),
        TETRIS_MASK(0b0010, 0b0011, 0b0001, 0b0000),
    },
    {
        TETRIS_MASK(0b0001, 0b0111, 0b0000, 0b0000),
        TETRIS_MASK(0b0110, 0b0010, 0b0010, 0b0000),
        TETRIS_MASK(0b0000, 0b0111, 0b0100, 0b0000),
        TETRIS_MASK(0b0010, 0b0010, 0b0011, 0b0000),
    },
    {
        TETRIS_MASK(0b0100, 0b0111, 0b0000, 0b0000),
        TETRIS_MASK(0b0010, 0b0010, 0b0110, 0b0000),
        TETRIS_MASK(0b0000, 0b0111, 0b0001, 0b0000),
        TETRIS_MASK(0b0011, 0b0010, 0b0010, 0b0000),
    },
};

const uint16_t TETRIS_COLORS[8] = {
    ST77XX_BLACK, ST77XX_CYAN, ST77XX_YELLOW, ST77XX_MAGENTA,
    ST77XX_GREEN, ST77XX_RED,  ST77XX_BLUE,   0xFD20,
};

struct TetrisPiece {
  uint8_t type;
  uint8_t rotation;
  int8_t x;
  int8_t y;
};

bool tetrisCanPlace(const TetrisPiece &piece, int dx, int dy, uint8_t rotation);

uint8_t tetrisBoard[TETRIS_ROWS][TETRIS_COLS];
uint8_t tetrisBag[TETRIS_PIECE_COUNT];
uint8_t tetrisBagIndex = TETRIS_PIECE_COUNT;
TetrisPiece tetrisActive{};
uint8_t tetrisNextType = 0;

unsigned long tetrisScore = 0;
unsigned int tetrisLines = 0;
unsigned int tetrisLevel = 1;
bool tetrisPaused = false;

unsigned long tetrisLastFall = 0;
bool tetrisUpLatched = false;
bool tetrisDownHeld = false;
unsigned long tetrisNextDownRepeat = 0;
int8_t tetrisHorizontalDirection = 0;
unsigned long tetrisNextHorizontalRepeat = 0;

uint16_t tetrisMask(uint8_t type, uint8_t rotation) {
  return pgm_read_word(&TETRIS_MASKS[type][rotation & 3]);
}

bool tetrisMaskCell(uint8_t type, uint8_t rotation, uint8_t row, uint8_t col) {
  return (tetrisMask(type, rotation) &
          (static_cast<uint16_t>(1) << (row * 4 + col))) != 0;
}

bool tetrisCanPlace(const TetrisPiece &piece, int dx, int dy,
                    uint8_t rotation) {
  for (uint8_t row = 0; row < 4; ++row) {
    for (uint8_t col = 0; col < 4; ++col) {
      if (!tetrisMaskCell(piece.type, rotation, row, col)) {
        continue;
      }

      const int boardX = piece.x + dx + col;
      const int boardY = piece.y + dy + row;

      if (boardX < 0 || boardX >= TETRIS_COLS || boardY >= TETRIS_ROWS) {
        return false;
      }

      if (boardY >= 0 && tetrisBoard[boardY][boardX] != 0) {
        return false;
      }
    }
  }

  return true;
}

bool tetrisMove(int dx, int dy) {
  if (!tetrisCanPlace(tetrisActive, dx, dy, tetrisActive.rotation)) {
    return false;
  }

  tetrisActive.x += dx;
  tetrisActive.y += dy;
  return true;
}

bool tetrisRotateClockwise() {
  const uint8_t nextRotation = (tetrisActive.rotation + 1) & 3;
  const int8_t kicks[] = {0, -1, 1, -2, 2};

  for (const int8_t kick : kicks) {
    if (tetrisCanPlace(tetrisActive, kick, 0, nextRotation)) {
      tetrisActive.x += kick;
      tetrisActive.rotation = nextRotation;
      return true;
    }
  }

  return false;
}

void tetrisFillBag() {
  for (uint8_t i = 0; i < TETRIS_PIECE_COUNT; ++i) {
    tetrisBag[i] = i;
  }

  for (int i = TETRIS_PIECE_COUNT - 1; i > 0; --i) {
    const int j = random(i + 1);
    const uint8_t temporary = tetrisBag[i];
    tetrisBag[i] = tetrisBag[j];
    tetrisBag[j] = temporary;
  }

  tetrisBagIndex = 0;
}

uint8_t tetrisTakeFromBag() {
  if (tetrisBagIndex >= TETRIS_PIECE_COUNT) {
    tetrisFillBag();
  }

  return tetrisBag[tetrisBagIndex++];
}

bool tetrisSpawnNext() {
  tetrisActive = {tetrisNextType, 0, 3, -1};
  tetrisNextType = tetrisTakeFromBag();
  return tetrisCanPlace(tetrisActive, 0, 0, tetrisActive.rotation);
}

bool tetrisLockPiece() {
  for (uint8_t row = 0; row < 4; ++row) {
    for (uint8_t col = 0; col < 4; ++col) {
      if (!tetrisMaskCell(tetrisActive.type, tetrisActive.rotation, row, col)) {
        continue;
      }

      const int boardX = tetrisActive.x + col;
      const int boardY = tetrisActive.y + row;

      if (boardY < 0) {
        return false;
      }

      tetrisBoard[boardY][boardX] = tetrisActive.type + 1;
    }
  }

  return true;
}

uint8_t tetrisClearLines() {
  uint8_t cleared = 0;
  int row = TETRIS_ROWS - 1;

  while (row >= 0) {
    bool full = true;
    for (int col = 0; col < TETRIS_COLS; ++col) {
      if (tetrisBoard[row][col] == 0) {
        full = false;
        break;
      }
    }

    if (!full) {
      --row;
      continue;
    }

    for (int moveRow = row; moveRow > 0; --moveRow) {
      memcpy(tetrisBoard[moveRow], tetrisBoard[moveRow - 1], TETRIS_COLS);
    }
    memset(tetrisBoard[0], 0, TETRIS_COLS);
    ++cleared;
  }

  return cleared;
}

unsigned long tetrisFallInterval() {
  const unsigned long reduction =
      static_cast<unsigned long>(tetrisLevel - 1) * 55UL;
  return reduction >= 650UL ? 150UL : 800UL - reduction;
}

//==================================================
// TETRIS RENDERING AND GAMEPLAY
//==================================================
void drawTetrisCell(int col, int row, uint16_t color) {
  if (row < 0) {
    return;
  }

  const int x = TETRIS_BOARD_X + col * TETRIS_CELL;
  const int y = TETRIS_BOARD_Y + row * TETRIS_CELL;
  tft.fillRect(x, y, TETRIS_CELL - 1, TETRIS_CELL - 1, color);
}

void drawTetrisBoard() {
  tft.fillRect(TETRIS_BOARD_X, TETRIS_BOARD_Y, TETRIS_COLS * TETRIS_CELL,
               TETRIS_ROWS * TETRIS_CELL, ST77XX_BLACK);

  tft.drawRect(TETRIS_BOARD_X - 1, TETRIS_BOARD_Y - 1,
               TETRIS_COLS * TETRIS_CELL + 2, TETRIS_ROWS * TETRIS_CELL + 2,
               ST77XX_WHITE);

  for (int row = 0; row < TETRIS_ROWS; ++row) {
    for (int col = 0; col < TETRIS_COLS; ++col) {
      const uint8_t colorId = tetrisBoard[row][col];
      if (colorId != 0) {
        drawTetrisCell(col, row, TETRIS_COLORS[colorId]);
      }
    }
  }
}

void drawTetrisActivePiece() {
  for (uint8_t row = 0; row < 4; ++row) {
    for (uint8_t col = 0; col < 4; ++col) {
      if (!tetrisMaskCell(tetrisActive.type, tetrisActive.rotation, row, col)) {
        continue;
      }

      const int boardY = tetrisActive.y + row;
      if (boardY >= 0) {
        drawTetrisCell(tetrisActive.x + col, boardY,
                       TETRIS_COLORS[tetrisActive.type + 1]);
      }
    }
  }
}

void drawTetrisNextPiece() {
  constexpr int previewX = 82;
  constexpr int previewY = 96;
  constexpr int previewCell = 5;

  tft.fillRect(76, 94, 28, 23, ST77XX_BLACK);

  for (uint8_t row = 0; row < 4; ++row) {
    for (uint8_t col = 0; col < 4; ++col) {
      if (!tetrisMaskCell(tetrisNextType, 0, row, col)) {
        continue;
      }

      tft.fillRect(previewX + col * previewCell, previewY + row * previewCell,
                   previewCell - 1, previewCell - 1,
                   TETRIS_COLORS[tetrisNextType + 1]);
    }
  }
}

void drawTetrisPanel() {
  tft.fillRect(68, 0, SCREEN_W - 68, SCREEN_H, ST77XX_BLACK);
  tft.setTextSize(1);

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(72, 4);
  tft.print("TETRIS");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(72, 18);
  tft.print("SCORE");
  tft.setCursor(72, 28);
  tft.print(tetrisScore);

  tft.setCursor(72, 40);
  tft.print("HIGH");
  tft.setCursor(72, 50);
  tft.print(max(static_cast<unsigned long>(tetrisHighScore), tetrisScore));

  tft.setCursor(72, 63);
  tft.print("LEVEL ");
  tft.print(tetrisLevel);

  tft.setCursor(72, 75);
  tft.print("LINES ");
  tft.print(tetrisLines);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(72, 86);
  tft.print("NEXT");
  drawTetrisNextPiece();

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(72, 120);
  tft.print("L:P R:DROP");
}

void drawTetris() {
  drawTetrisBoard();
  drawTetrisActivePiece();
  drawTetrisPanel();
}

void drawTetrisPauseOverlay() {
  tft.fillRect(13, 54, 43, 18, ST77XX_BLACK);
  tft.drawRect(13, 54, 43, 18, ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(17, 60);
  tft.print("PAUSED");
}

void drawTetrisGameOver() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(34, 12);
  tft.print("TETRIS");

  tft.setTextColor(ST77XX_RED);
  tft.setCursor(25, 36);
  tft.print("GAME OVER");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(44, 70);
  tft.print("Score: ");
  tft.print(tetrisScore);

  tft.setCursor(32, 84);
  tft.print("Tetris High: ");
  tft.print(tetrisHighScore);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(35, 102);
  tft.print("RIGHT: RESTART");
  tft.setCursor(47, 114);
  tft.print("LEFT: MENU");
}

void endTetrisGame() {
  vibrateFor(300);

  if (tetrisScore > tetrisHighScore) {
    tetrisHighScore = static_cast<unsigned int>(tetrisScore);
    prefs.putUInt("tetrisHigh", tetrisHighScore);
  }

  appState = TETRIS_GAME_OVER;
  beepGameOver();
  drawTetrisGameOver();
}

void tetrisLandActive() {
  if (!tetrisLockPiece()) {
    endTetrisGame();
    return;
  }

  constexpr unsigned int lineScores[5] = {0, 100, 300, 500, 800};
  const uint8_t cleared = tetrisClearLines();
  if (cleared > 0) {
    vibrateFor(40);
  }
  tetrisScore += lineScores[cleared];
  tetrisLines += cleared;
  tetrisLevel = 1 + tetrisLines / 10;

  if (!tetrisSpawnNext()) {
    endTetrisGame();
    return;
  }

  tetrisLastFall = millis();
  drawTetris();
}

void resetTetris() {
  memset(tetrisBoard, 0, sizeof(tetrisBoard));
  tetrisBagIndex = TETRIS_PIECE_COUNT;
  tetrisNextType = tetrisTakeFromBag();

  tetrisScore = 0;
  tetrisLines = 0;
  tetrisLevel = 1;
  tetrisPaused = false;
  tetrisUpLatched = false;
  tetrisDownHeld = false;
  tetrisHorizontalDirection = 0;
  tetrisLastFall = millis();
  tetrisNextDownRepeat = 0;
  tetrisNextHorizontalRepeat = 0;

  tft.fillScreen(ST77XX_BLACK);

  if (!tetrisSpawnNext()) {
    endTetrisGame();
    return;
  }

  appState = TETRIS_PLAYING;
  drawTetris();
}

void updateTetrisHorizontal(const InputFrame &input, unsigned long now) {
  const int8_t direction = input.left ? -1 : (input.right ? 1 : 0);

  if (direction == 0) {
    tetrisHorizontalDirection = 0;
    return;
  }

  if (direction != tetrisHorizontalDirection) {
    tetrisHorizontalDirection = direction;
    tetrisMove(direction, 0);
    tetrisNextHorizontalRepeat = now + 180;
    drawTetris();
    return;
  }

  if (now >= tetrisNextHorizontalRepeat) {
    tetrisMove(direction, 0);
    tetrisNextHorizontalRepeat = now + 85;
    drawTetris();
  }
}

void updateTetrisSoftDrop(const InputFrame &input, unsigned long now) {
  if (!input.down) {
    tetrisDownHeld = false;
    return;
  }

  if (!tetrisDownHeld || now >= tetrisNextDownRepeat) {
    tetrisDownHeld = true;
    tetrisNextDownRepeat = now + 55;

    if (!tetrisMove(0, 1)) {
      tetrisLandActive();
      return;
    }

    drawTetris();
  }
}

void updateTetris(const InputFrame &input) {
  if (input.leftPressed) {
    tetrisPaused = !tetrisPaused;
    if (tetrisPaused) {
      drawTetrisPauseOverlay();
    } else {
      tetrisLastFall = millis();
      drawTetris();
    }
  }

  if (tetrisPaused) {
    return;
  }

  const unsigned long now = millis();

  if (!input.up) {
    tetrisUpLatched = false;
  } else if (!tetrisUpLatched) {
    tetrisUpLatched = true;
    if (tetrisRotateClockwise()) {
      drawTetris();
    }
  }

  if (input.rightPressed) {
    while (tetrisMove(0, 1)) {
    }
    tetrisLandActive();
    return;
  }

  updateTetrisHorizontal(input, now);
  updateTetrisSoftDrop(input, now);

  if (appState != TETRIS_PLAYING) {
    return;
  }

  if (now - tetrisLastFall >= tetrisFallInterval()) {
    tetrisLastFall = now;

    if (!tetrisMove(0, 1)) {
      tetrisLandActive();
      return;
    }

    drawTetris();
  }
}

void updateTetrisGameOver(const InputFrame &input) {
  if (input.rightPressed) {
    resetTetris();
  } else if (input.leftPressed) {
    enterMenu();
  }
}

//==================================================
// DOOM-LITE
//==================================================
constexpr uint8_t DOOM_MAP_W = 16;
constexpr uint8_t DOOM_MAP_H = 16;
constexpr uint8_t DOOM_RAYS = 80;
constexpr uint8_t DOOM_COLUMN_W = 2;
constexpr uint8_t DOOM_VIEW_H = 104;
constexpr uint8_t DOOM_HUD_Y = 104;
constexpr uint8_t DOOM_ENEMY_COUNT = 5;
constexpr uint8_t DOOM_PICKUP_COUNT = DOOM_ENEMY_COUNT;
constexpr uint8_t DOOM_SPRITE_COUNT = DOOM_ENEMY_COUNT + DOOM_PICKUP_COUNT;

constexpr float DOOM_FOV = PI / 3.0f;
constexpr float DOOM_MOVE_STEP = 0.10f;
constexpr float DOOM_TURN_STEP = 0.09f;
constexpr float DOOM_PLAYER_RADIUS = 0.20f;
constexpr unsigned long DOOM_FRAME_MS = 40;
constexpr unsigned long DOOM_ENEMY_TICK_MS = 120;

static_assert(DOOM_RAYS * DOOM_COLUMN_W == SCREEN_W,
              "Doom ray columns must cover the display width");

const uint8_t DOOM_MAP[DOOM_MAP_H][DOOM_MAP_W] PROGMEM = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 1, 0, 1, 2, 0, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1},
    {1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1},
    {1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

struct DoomRayHit {
  float distance;
  uint8_t tile;
  bool verticalSide;
};

struct DoomEnemy {
  float x;
  float y;
  int8_t health;
  bool alive;
  unsigned long nextAttackAt;
};

struct DoomPickup {
  float x;
  float y;
  bool active;
};

struct DoomSpriteRef {
  float distance;
  bool pickup;
  uint8_t index;
};

DoomRayHit doomCastRay(float angle);

float doomPlayerX = 1.5f;
float doomPlayerY = 1.5f;
float doomPlayerAngle = 0.0f;
float doomDepth[DOOM_RAYS];

DoomEnemy doomEnemies[DOOM_ENEMY_COUNT];
DoomPickup doomPickups[DOOM_PICKUP_COUNT];
DoomSpriteRef doomSprites[DOOM_SPRITE_COUNT];

int doomHealth = 100;
int doomAmmo = 12;
int doomScore = 0;
uint8_t doomEnemiesRemaining = DOOM_ENEMY_COUNT;
uint8_t doomSpriteCount = 0;
bool doomPaused = false;
bool doomWon = false;

unsigned long doomLastFrame = 0;
unsigned long doomLastEnemyTick = 0;
unsigned long doomNextShotAt = 0;

float doomNormalizeAngle(float angle) {
  while (angle > PI) {
    angle -= TWO_PI;
  }
  while (angle < -PI) {
    angle += TWO_PI;
  }
  return angle;
}

uint8_t doomTileAt(int x, int y) {
  if (x < 0 || x >= DOOM_MAP_W || y < 0 || y >= DOOM_MAP_H) {
    return 1;
  }

  return pgm_read_byte(&DOOM_MAP[y][x]);
}

bool doomPositionClear(float x, float y, float radius) {
  return doomTileAt(static_cast<int>(x - radius),
                    static_cast<int>(y - radius)) == 0 &&
         doomTileAt(static_cast<int>(x + radius),
                    static_cast<int>(y - radius)) == 0 &&
         doomTileAt(static_cast<int>(x - radius),
                    static_cast<int>(y + radius)) == 0 &&
         doomTileAt(static_cast<int>(x + radius),
                    static_cast<int>(y + radius)) == 0;
}

bool doomEnemyPositionClear(uint8_t movingIndex, float x, float y) {
  if (!doomPositionClear(x, y, 0.18f)) {
    return false;
  }

  for (uint8_t index = 0; index < DOOM_ENEMY_COUNT; ++index) {
    if (index == movingIndex || !doomEnemies[index].alive) {
      continue;
    }

    const float deltaX = doomEnemies[index].x - x;
    const float deltaY = doomEnemies[index].y - y;
    if (deltaX * deltaX + deltaY * deltaY < 0.2025f) {
      return false;
    }
  }

  return true;
}

uint16_t doomShadeColor(uint16_t color, int brightness) {
  brightness = constrain(brightness, 35, 255);

  uint8_t red = (color >> 11) & 0x1F;
  uint8_t green = (color >> 5) & 0x3F;
  uint8_t blue = color & 0x1F;

  red = static_cast<uint8_t>(red * brightness / 255);
  green = static_cast<uint8_t>(green * brightness / 255);
  blue = static_cast<uint8_t>(blue * brightness / 255);

  return (static_cast<uint16_t>(red) << 11) |
         (static_cast<uint16_t>(green) << 5) | blue;
}

DoomRayHit doomCastRay(float angle) {
  const float directionX = cosf(angle);
  const float directionY = sinf(angle);

  int mapX = static_cast<int>(doomPlayerX);
  int mapY = static_cast<int>(doomPlayerY);

  const float deltaX =
      fabsf(directionX) < 0.0001f ? 1000000.0f : fabsf(1.0f / directionX);
  const float deltaY =
      fabsf(directionY) < 0.0001f ? 1000000.0f : fabsf(1.0f / directionY);

  const int stepX = directionX < 0.0f ? -1 : 1;
  const int stepY = directionY < 0.0f ? -1 : 1;

  float sideX = directionX < 0.0f ? (doomPlayerX - mapX) * deltaX
                                  : (mapX + 1.0f - doomPlayerX) * deltaX;
  float sideY = directionY < 0.0f ? (doomPlayerY - mapY) * deltaY
                                  : (mapY + 1.0f - doomPlayerY) * deltaY;

  bool verticalSide = false;
  uint8_t tile = 0;

  for (uint8_t step = 0; step < 64; ++step) {
    if (sideX < sideY) {
      sideX += deltaX;
      mapX += stepX;
      verticalSide = true;
    } else {
      sideY += deltaY;
      mapY += stepY;
      verticalSide = false;
    }

    tile = doomTileAt(mapX, mapY);
    if (tile != 0) {
      float distance;
      if (verticalSide) {
        distance = (mapX - doomPlayerX + (1 - stepX) * 0.5f) / directionX;
      } else {
        distance = (mapY - doomPlayerY + (1 - stepY) * 0.5f) / directionY;
      }

      return {fabsf(distance), tile, verticalSide};
    }
  }

  return {20.0f, 1, false};
}

void doomMovePlayer(const InputFrame &input) {
  float forward = 0.0f;
  float strafe = 0.0f;

  if (input.x > JOYSTICK_HIGH) {
    forward = 1.0f;
  } else if (input.x < JOYSTICK_LOW) {
    forward = -1.0f;
  }

  if (input.y > JOYSTICK_HIGH) {
    strafe = 1.0f;
  } else if (input.y < JOYSTICK_LOW) {
    strafe = -1.0f;
  }

  if (forward != 0.0f && strafe != 0.0f) {
    forward *= 0.7071067f;
    strafe *= 0.7071067f;
  }

  const float movementX =
      cosf(doomPlayerAngle) * forward * DOOM_MOVE_STEP +
      cosf(doomPlayerAngle + PI / 2.0f) * strafe * DOOM_MOVE_STEP;
  const float movementY =
      sinf(doomPlayerAngle) * forward * DOOM_MOVE_STEP +
      sinf(doomPlayerAngle + PI / 2.0f) * strafe * DOOM_MOVE_STEP;

  if (doomPositionClear(doomPlayerX + movementX, doomPlayerY,
                        DOOM_PLAYER_RADIUS)) {
    doomPlayerX += movementX;
  }

  if (doomPositionClear(doomPlayerX, doomPlayerY + movementY,
                        DOOM_PLAYER_RADIUS)) {
    doomPlayerY += movementY;
  }

  if (input.turnLeft) {
    doomPlayerAngle -= DOOM_TURN_STEP;
  }
  if (input.turnRight) {
    doomPlayerAngle += DOOM_TURN_STEP;
  }
  doomPlayerAngle = doomNormalizeAngle(doomPlayerAngle);
}

void doomRenderWorld() {
  constexpr uint16_t ceilingColor = 0x18C3;
  constexpr uint16_t floorColor = 0x3186;
  constexpr uint16_t blueWall = 0x6B4D;
  constexpr uint16_t redWall = 0xA145;

  tft.startWrite();

  for (uint8_t ray = 0; ray < DOOM_RAYS; ++ray) {
    const float rayAngle =
        doomPlayerAngle - DOOM_FOV / 2.0f + (ray + 0.5f) * DOOM_FOV / DOOM_RAYS;
    const DoomRayHit hit = doomCastRay(rayAngle);
    const float corrected =
        fmaxf(hit.distance * cosf(rayAngle - doomPlayerAngle), 0.05f);
    doomDepth[ray] = corrected;

    const int wallHeight =
        constrain(static_cast<int>(DOOM_VIEW_H / corrected), 1, DOOM_VIEW_H);
    const int wallTop = (DOOM_VIEW_H - wallHeight) / 2;
    const int wallBottom = wallTop + wallHeight;
    const int brightness = 245 - min(175, static_cast<int>(corrected * 22.0f)) -
                           (hit.verticalSide ? 28 : 0);
    const uint16_t baseColor = hit.tile == 2 ? redWall : blueWall;
    const uint16_t wallColor = doomShadeColor(baseColor, brightness);
    const int screenX = ray * DOOM_COLUMN_W;

    if (wallTop > 0) {
      tft.writeFillRect(screenX, 0, DOOM_COLUMN_W, wallTop, ceilingColor);
    }

    tft.writeFillRect(screenX, wallTop, DOOM_COLUMN_W, wallHeight, wallColor);

    if (wallBottom < DOOM_VIEW_H) {
      tft.writeFillRect(screenX, wallBottom, DOOM_COLUMN_W,
                        DOOM_VIEW_H - wallBottom, floorColor);
    }
  }

  tft.endWrite();
}

bool doomHasLineOfSight(float fromX, float fromY, float toX, float toY) {
  const float deltaX = toX - fromX;
  const float deltaY = toY - fromY;
  const float distance = sqrtf(deltaX * deltaX + deltaY * deltaY);
  const int steps = max(1, static_cast<int>(distance / 0.10f));

  for (int step = 1; step < steps; ++step) {
    const float fraction = static_cast<float>(step) / static_cast<float>(steps);
    const float x = fromX + deltaX * fraction;
    const float y = fromY + deltaY * fraction;

    if (doomTileAt(static_cast<int>(x), static_cast<int>(y)) != 0) {
      return false;
    }
  }

  return true;
}

void doomResetEnemies() {
  const float starts[DOOM_ENEMY_COUNT][2] = {
      {3.5f, 1.5f}, {6.5f, 5.5f}, {13.5f, 5.5f}, {8.5f, 11.5f}, {13.5f, 13.5f},
  };

  for (uint8_t index = 0; index < DOOM_ENEMY_COUNT; ++index) {
    doomEnemies[index] = {
        starts[index][0], starts[index][1], 2, true, 0,
    };
    doomPickups[index] = {
        starts[index][0],
        starts[index][1],
        false,
    };
  }

  doomEnemiesRemaining = DOOM_ENEMY_COUNT;
}

void doomFinish(bool won);

void doomUpdateEnemies(unsigned long now) {
  for (uint8_t index = 0; index < DOOM_ENEMY_COUNT; ++index) {
    DoomEnemy &enemy = doomEnemies[index];
    if (!enemy.alive) {
      continue;
    }

    const float deltaX = doomPlayerX - enemy.x;
    const float deltaY = doomPlayerY - enemy.y;
    const float distance = sqrtf(deltaX * deltaX + deltaY * deltaY);

    if (!doomHasLineOfSight(enemy.x, enemy.y, doomPlayerX, doomPlayerY)) {
      continue;
    }

    if (distance <= 0.70f) {
      if (enemy.nextAttackAt == 0 ||
          static_cast<long>(now - enemy.nextAttackAt) >= 0) {
        doomHealth = max(0, doomHealth - 10);
        enemy.nextAttackAt = now + 900;
        tone(BUZZER_PIN, 320, 90);
        vibrateFor(40);
      }
      continue;
    }

    const float moveX = deltaX / distance * 0.045f;
    const float moveY = deltaY / distance * 0.045f;

    if (doomEnemyPositionClear(index, enemy.x + moveX, enemy.y)) {
      enemy.x += moveX;
    }
    if (doomEnemyPositionClear(index, enemy.x, enemy.y + moveY)) {
      enemy.y += moveY;
    }
  }
}

void doomShoot(unsigned long now) {
  if (doomNextShotAt != 0 && static_cast<long>(now - doomNextShotAt) < 0) {
    return;
  }

  doomNextShotAt = now + 220;

  if (doomAmmo <= 0) {
    tone(BUZZER_PIN, 150, 80);
    return;
  }

  --doomAmmo;
  tone(BUZZER_PIN, 1700, 45);

  int selected = -1;
  float selectedDistance = 1000000.0f;

  for (uint8_t index = 0; index < DOOM_ENEMY_COUNT; ++index) {
    const DoomEnemy &enemy = doomEnemies[index];
    if (!enemy.alive) {
      continue;
    }

    const float deltaX = enemy.x - doomPlayerX;
    const float deltaY = enemy.y - doomPlayerY;
    const float distance = sqrtf(deltaX * deltaX + deltaY * deltaY);
    const float enemyAngle = atan2f(deltaY, deltaX);
    const float angleDifference =
        fabsf(doomNormalizeAngle(enemyAngle - doomPlayerAngle));

    if (angleDifference > 0.12f || distance >= selectedDistance ||
        !doomHasLineOfSight(doomPlayerX, doomPlayerY, enemy.x, enemy.y)) {
      continue;
    }

    selected = index;
    selectedDistance = distance;
  }

  if (selected < 0) {
    return;
  }

  DoomEnemy &enemy = doomEnemies[selected];
  --enemy.health;
  tone(BUZZER_PIN, 1050, 40);

  if (enemy.health > 0) {
    return;
  }

  enemy.alive = false;
  doomScore += 100;
  --doomEnemiesRemaining;
  tone(BUZZER_PIN, 1950, 90);

  doomPickups[selected] = {
      enemy.x,
      enemy.y,
      doomAmmo < 8 || random(100) < 35,
  };

  if (doomEnemiesRemaining == 0) {
    doomFinish(true);
  }
}

void doomCollectPickups() {
  for (uint8_t index = 0; index < DOOM_PICKUP_COUNT; ++index) {
    DoomPickup &pickup = doomPickups[index];
    if (!pickup.active) {
      continue;
    }

    const float deltaX = pickup.x - doomPlayerX;
    const float deltaY = pickup.y - doomPlayerY;

    if (deltaX * deltaX + deltaY * deltaY > 0.1225f) {
      continue;
    }

    doomAmmo = min(40, doomAmmo + 6);
    pickup.active = false;
    tone(BUZZER_PIN, 1500, 70);
    vibrateFor(40);
  }
}

void doomBuildSpriteList() {
  doomSpriteCount = 0;

  for (uint8_t index = 0; index < DOOM_ENEMY_COUNT; ++index) {
    if (!doomEnemies[index].alive) {
      continue;
    }

    const float deltaX = doomEnemies[index].x - doomPlayerX;
    const float deltaY = doomEnemies[index].y - doomPlayerY;
    doomSprites[doomSpriteCount++] = {
        sqrtf(deltaX * deltaX + deltaY * deltaY),
        false,
        index,
    };
  }

  for (uint8_t index = 0; index < DOOM_PICKUP_COUNT; ++index) {
    if (!doomPickups[index].active) {
      continue;
    }

    const float deltaX = doomPickups[index].x - doomPlayerX;
    const float deltaY = doomPickups[index].y - doomPlayerY;
    doomSprites[doomSpriteCount++] = {
        sqrtf(deltaX * deltaX + deltaY * deltaY),
        true,
        index,
    };
  }

  for (uint8_t index = 1; index < doomSpriteCount; ++index) {
    const DoomSpriteRef key = doomSprites[index];
    int position = index - 1;

    while (position >= 0 && doomSprites[position].distance < key.distance) {
      doomSprites[position + 1] = doomSprites[position];
      --position;
    }

    doomSprites[position + 1] = key;
  }
}

void doomRenderSprites() {
  doomBuildSpriteList();

  for (uint8_t spriteIndex = 0; spriteIndex < doomSpriteCount; ++spriteIndex) {
    const DoomSpriteRef &sprite = doomSprites[spriteIndex];
    const float entityX = sprite.pickup ? doomPickups[sprite.index].x
                                        : doomEnemies[sprite.index].x;
    const float entityY = sprite.pickup ? doomPickups[sprite.index].y
                                        : doomEnemies[sprite.index].y;

    const float deltaX = entityX - doomPlayerX;
    const float deltaY = entityY - doomPlayerY;
    const float entityAngle = atan2f(deltaY, deltaX);
    const float relativeAngle =
        doomNormalizeAngle(entityAngle - doomPlayerAngle);

    if (fabsf(relativeAngle) > DOOM_FOV * 0.65f) {
      continue;
    }

    const float projectedDistance =
        fmaxf(sprite.distance * cosf(relativeAngle), 0.05f);
    const int centerX = static_cast<int>(
        SCREEN_W / 2.0f +
        tanf(relativeAngle) * (SCREEN_W / 2.0f) / tanf(DOOM_FOV / 2.0f));
    const int spriteHeight =
        constrain(static_cast<int>(DOOM_VIEW_H / projectedDistance),
                  sprite.pickup ? 4 : 8, sprite.pickup ? 28 : DOOM_VIEW_H);
    const int spriteWidth =
        sprite.pickup ? max(4, spriteHeight) : max(5, spriteHeight / 2);
    const int top = DOOM_VIEW_H / 2 - spriteHeight / 2 +
                    (sprite.pickup ? spriteHeight / 3 : 0);
    const int left = centerX - spriteWidth / 2;
    const uint16_t color =
        sprite.pickup
            ? ST77XX_YELLOW
            : (doomEnemies[sprite.index].health > 1 ? ST77XX_RED : 0xFD20);

    for (int localX = 0; localX < spriteWidth; localX += 2) {
      const int screenX = left + localX;
      if (screenX < 0 || screenX >= SCREEN_W) {
        continue;
      }

      const int ray = constrain(screenX / DOOM_COLUMN_W, 0, DOOM_RAYS - 1);
      if (projectedDistance >= doomDepth[ray] + 0.04f) {
        continue;
      }

      const float edgeDistance =
          fabsf(localX - spriteWidth / 2.0f) / max(1.0f, spriteWidth / 2.0f);
      const int inset =
          sprite.pickup ? 0
                        : static_cast<int>(edgeDistance * spriteHeight * 0.12f);
      const int drawTop = constrain(top + inset, 0, DOOM_VIEW_H - 1);
      const int drawBottom =
          constrain(top + spriteHeight - inset, 0, DOOM_VIEW_H);

      if (drawBottom > drawTop) {
        tft.fillRect(
            screenX, drawTop, 2, drawBottom - drawTop,
            doomShadeColor(
                color,
                245 - min(120, static_cast<int>(projectedDistance * 18.0f))));
      }
    }
  }
}

void doomDrawHud() {
  tft.fillRect(0, DOOM_HUD_Y, SCREEN_W, SCREEN_H - DOOM_HUD_Y, 0x2104);
  tft.drawFastHLine(0, DOOM_HUD_Y, SCREEN_W, ST77XX_RED);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(2, 108);
  tft.print("HP");
  tft.setTextColor(doomHealth <= 30 ? ST77XX_RED : ST77XX_GREEN);
  tft.setCursor(17, 108);
  tft.print(doomHealth);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(48, 108);
  tft.print("AM");
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(63, 108);
  tft.print(doomAmmo);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(88, 108);
  tft.print("SC");
  tft.setCursor(103, 108);
  tft.print(doomScore);

  tft.setCursor(2, 119);
  tft.print("ENEMIES ");
  tft.setTextColor(ST77XX_RED);
  tft.print(doomEnemiesRemaining);

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(102, 119);
  tft.print("HI ");
  tft.print(doomHighScore);
}

void doomDrawWeaponAndCrosshair() {
  const int centerX = SCREEN_W / 2;
  const int centerY = DOOM_VIEW_H / 2;

  tft.drawFastHLine(centerX - 7, centerY, 5, ST77XX_WHITE);
  tft.drawFastHLine(centerX + 3, centerY, 5, ST77XX_WHITE);
  tft.drawFastVLine(centerX, centerY - 7, 5, ST77XX_WHITE);
  tft.drawFastVLine(centerX, centerY + 3, 5, ST77XX_WHITE);

  tft.fillRect(centerX - 8, 91, 16, 13, 0x39E7);
  tft.fillRect(centerX - 4, 84, 8, 13, 0x7BEF);
  tft.drawRect(centerX - 4, 84, 8, 13, ST77XX_BLACK);
  tft.fillRect(centerX - 2, 81, 4, 5, 0x4208);
}

void doomRenderFrame() {
  doomRenderWorld();
  doomRenderSprites();
  doomDrawWeaponAndCrosshair();
  doomDrawHud();
}

void doomDrawPauseOverlay() {
  tft.fillRect(52, 43, 56, 20, ST77XX_BLACK);
  tft.drawRect(52, 43, 56, 20, ST77XX_YELLOW);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(62, 49);
  tft.print("PAUSED");
}

void doomDrawFinishScreen() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(doomWon ? ST77XX_GREEN : ST77XX_RED);
  tft.setCursor(doomWon ? 14 : 25, 16);
  tft.print(doomWon ? "LEVEL CLEAR" : "GAME OVER");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(46, 55);
  tft.print("Score: ");
  tft.print(doomScore);
  tft.setCursor(30, 70);
  tft.print("Doom High: ");
  tft.print(doomHighScore);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(35, 96);
  tft.print("RIGHT: RESTART");
  tft.setCursor(47, 110);
  tft.print("LEFT: MENU");
}

void doomFinish(bool won) {
  if (appState != DOOM_PLAYING) {
    return;
  }

  doomWon = won;

  if (doomScore > static_cast<int>(doomHighScore)) {
    doomHighScore = static_cast<unsigned int>(doomScore);
    prefs.putUInt("doomHigh", doomHighScore);
  }

  appState = DOOM_GAME_OVER;

  if (won) {
    tone(BUZZER_PIN, 1200, 80);
    delay(90);
    tone(BUZZER_PIN, 1800, 160);
  } else {
    beepGameOver();
    vibrateFor(300);
  }

  doomDrawFinishScreen();
}

void resetDoom() {
  doomPlayerX = 1.5f;
  doomPlayerY = 1.5f;
  doomPlayerAngle = 0.0f;
  doomHealth = 100;
  doomAmmo = 12;
  doomScore = 0;
  doomPaused = false;
  doomWon = false;
  doomNextShotAt = 0;
  doomLastFrame = millis();
  doomLastEnemyTick = doomLastFrame;
  doomResetEnemies();

  appState = DOOM_PLAYING;
  tft.fillScreen(ST77XX_BLACK);
  doomRenderFrame();
}

void updateDoom(const InputFrame &input) {
  const unsigned long now = millis();

  if (input.leftPressed) {
    doomPaused = !doomPaused;

    if (doomPaused) {
      doomDrawPauseOverlay();
    } else {
      doomLastFrame = now;
      doomLastEnemyTick = now;
      for (uint8_t index = 0; index < DOOM_ENEMY_COUNT; ++index) {
        if (doomEnemies[index].alive) {
          doomEnemies[index].nextAttackAt = now + 500;
        }
      }
      doomRenderFrame();
    }
  }

  if (doomPaused) {
    return;
  }

  if (input.rightPressed) {
    doomShoot(now);
    if (appState != DOOM_PLAYING) {
      return;
    }
  }

  if (now - doomLastFrame < DOOM_FRAME_MS) {
    return;
  }
  doomLastFrame = now;

  doomMovePlayer(input);

  if (now - doomLastEnemyTick >= DOOM_ENEMY_TICK_MS) {
    doomLastEnemyTick = now;
    doomUpdateEnemies(now);
  }

  doomCollectPickups();

  if (doomHealth <= 0) {
    doomFinish(false);
    return;
  }

  if (doomEnemiesRemaining == 0) {
    doomFinish(true);
    return;
  }

  doomRenderFrame();
}

void updateDoomGameOver(const InputFrame &input) {
  if (input.rightPressed) {
    resetDoom();
  } else if (input.leftPressed) {
    enterMenu();
  }
}

//==================================================
// RACER
//==================================================
constexpr uint8_t RACER_TRAFFIC_CAPACITY = 6;
constexpr uint8_t RACER_PICKUP_CAPACITY = 3;
constexpr uint8_t RACER_SPRITE_CAPACITY =
    RACER_TRAFFIC_CAPACITY + RACER_PICKUP_CAPACITY;
constexpr uint8_t RACER_HORIZON_Y = 30;
constexpr unsigned long RACER_FRAME_MS = 33;
constexpr float RACER_NORMAL_MAX_SPEED = 80.0f;
constexpr float RACER_TURBO_MAX_SPEED = 110.0f;
constexpr float RACER_TURBO_USE_PER_SECOND = 30.0f;
constexpr float RACER_FAR_DISTANCE = 140.0f;
constexpr float RACER_RECYCLE_DISTANCE = -6.0f;
constexpr float RACER_COLLISION_DISTANCE = 6.0f;
constexpr float RACER_LANE_SPACING = 0.62f;

static_assert(RACER_TRAFFIC_CAPACITY == 6, "Racer must use six traffic slots");
static_assert(RACER_PICKUP_CAPACITY == 3, "Racer must use three pickup slots");
static_assert(RACER_FRAME_MS == 33, "Racer frame interval must remain 33 ms");

struct RacerTraffic {
  float lateral;
  float distance;
  float speed;
  uint16_t color;
  bool active;
  bool cleanPass;
};

struct RacerPickup {
  float lateral;
  float distance;
  bool active;
};

struct RacerSpriteRef {
  float distance;
  bool pickup;
  uint8_t index;
};

RacerTraffic racerTraffic[RACER_TRAFFIC_CAPACITY];
RacerPickup racerPickups[RACER_PICKUP_CAPACITY];
RacerSpriteRef racerSprites[RACER_SPRITE_CAPACITY];
unsigned long racerTrafficRetryAt[RACER_TRAFFIC_CAPACITY];

float racerPlayerLateral = 0.0f;
float racerSpeed = 25.0f;
float racerWorldDistance = 0.0f;
float racerTurbo = 50.0f;
int racerDurability = 100;
unsigned long racerOvertakeScore = 0;
unsigned long racerScore = 0;
uint8_t racerActiveTraffic = 3;
uint8_t racerSpriteCount = 0;
bool racerPaused = false;
bool racerTurboActive = false;
bool racerTurboReady = false;

unsigned long racerLastFrame = 0;
unsigned long racerCollisionSafeUntil = 0;
unsigned long racerNextPickupAt = 0;
unsigned long racerPauseStartedAt = 0;

float racerClamp(float value, float low, float high) {
  return fmaxf(low, fminf(value, high));
}

float racerLanePosition(int8_t lane) {
  return racerClamp(lane, -1, 1) * RACER_LANE_SPACING;
}

float racerRoadCurveAt(float distance) {
  const int milestone = min(3, static_cast<int>(racerWorldDistance / 750.0f));
  const float strength = 0.24f + milestone * 0.09f;

  return (sinf(distance * 0.010f) * 0.72f + sinf(distance * 0.0031f) * 0.28f) *
         strength;
}

void racerUpdateDriving(const InputFrame &input, float deltaSeconds) {
  if (!input.rightHeld) {
    racerTurboReady = true;
  }

  const bool accelerating = input.x > JOYSTICK_HIGH;
  const bool braking = input.x < JOYSTICK_LOW;
  const bool wantsTurbo = racerTurboReady && input.rightHeld &&
                          racerTurbo > 0.0f && racerSpeed > 15.0f;

  if (accelerating) {
    racerSpeed += 35.0f * deltaSeconds;
  } else if (braking) {
    racerSpeed -= 48.0f * deltaSeconds;
  } else {
    racerSpeed -= 12.0f * deltaSeconds;
  }

  if (wantsTurbo) {
    if (!racerTurboActive) {
      tone(BUZZER_PIN, 1450, 70);
    }
    racerTurboActive = true;
    racerTurbo =
        fmaxf(0.0f, racerTurbo - RACER_TURBO_USE_PER_SECOND * deltaSeconds);
    racerSpeed += 42.0f * deltaSeconds;
  } else {
    racerTurboActive = false;
  }

  const float speedLimit =
      wantsTurbo ? RACER_TURBO_MAX_SPEED : RACER_NORMAL_MAX_SPEED;
  racerSpeed = racerClamp(racerSpeed, 0.0f, speedLimit);

  float steering = 0.0f;
  if (input.y > JOYSTICK_HIGH) {
    steering = 1.0f;
  } else if (input.y < JOYSTICK_LOW) {
    steering = -1.0f;
  }

  const float steeringRate = 0.55f + racerSpeed / RACER_NORMAL_MAX_SPEED;
  racerPlayerLateral = racerClamp(
      racerPlayerLateral + steering * steeringRate * deltaSeconds, -1.0f, 1.0f);

  if (fabsf(racerPlayerLateral) > 0.92f) {
    racerSpeed = fmaxf(0.0f, racerSpeed - 40.0f * deltaSeconds);
  }

  racerWorldDistance += racerSpeed * deltaSeconds * 0.60f;
  racerScore = static_cast<unsigned long>(racerWorldDistance / 5.0f) +
               racerOvertakeScore;
}

void racerRoadGeometry(int screenY, float &depth, int &center, int &halfWidth) {
  const float viewHeight = static_cast<float>(SCREEN_H - RACER_HORIZON_Y);
  const float proximity = racerClamp(
      static_cast<float>(screenY - RACER_HORIZON_Y) / viewHeight, 0.0f, 1.0f);

  depth = RACER_FAR_DISTANCE * (1.0f - sqrtf(proximity));
  halfWidth = 10 + static_cast<int>(proximity * 70.0f);

  const float curveDifference = racerRoadCurveAt(racerWorldDistance + depth) -
                                racerRoadCurveAt(racerWorldDistance);
  const float curveOffset = curveDifference * (1.0f - proximity) * 72.0f;
  const float playerOffset = -racerPlayerLateral * halfWidth * 0.48f;
  center = static_cast<int>(SCREEN_W / 2.0f + curveOffset + playerOffset);
}

void racerRenderRoad() {
  tft.fillRect(0, 0, SCREEN_W, RACER_HORIZON_Y, 0x4D7F);
  tft.fillCircle(132, 22, 6, ST77XX_YELLOW);

  tft.startWrite();

  for (int y = RACER_HORIZON_Y; y < SCREEN_H; y += 2) {
    float depth;
    int center;
    int halfWidth;
    racerRoadGeometry(y, depth, center, halfWidth);

    const bool alternate =
        (static_cast<int>((racerWorldDistance + depth) / 9.0f) & 1) != 0;
    const uint16_t grass = alternate ? 0x0440 : 0x05A0;
    const uint16_t shoulder = alternate ? ST77XX_WHITE : ST77XX_RED;
    const int left = center - halfWidth;
    const int right = center + halfWidth;
    const int clippedLeft = constrain(left, 0, SCREEN_W);
    const int clippedRight = constrain(right, 0, SCREEN_W);
    const int shoulderWidth = max(2, halfWidth / 12);

    tft.writeFillRect(0, y, SCREEN_W, 2, grass);

    if (clippedRight > clippedLeft) {
      tft.writeFillRect(clippedLeft, y, clippedRight - clippedLeft, 2, 0x4208);

      const int leftShoulderRight =
          min(clippedRight, clippedLeft + shoulderWidth);
      tft.writeFillRect(clippedLeft, y, leftShoulderRight - clippedLeft, 2,
                        shoulder);

      const int rightShoulderLeft =
          max(clippedLeft, clippedRight - shoulderWidth);
      tft.writeFillRect(rightShoulderLeft, y, clippedRight - rightShoulderLeft,
                        2, shoulder);
    }

    if (alternate && halfWidth > 18) {
      for (int divider = -1; divider <= 1; divider += 2) {
        const int markerX = center + divider * halfWidth / 3;
        if (markerX >= 0 && markerX < SCREEN_W - 1) {
          tft.writeFillRect(markerX, y, 2, 2, ST77XX_YELLOW);
        }
      }
    }
  }

  tft.endWrite();
}

bool racerProjectEntity(float distance, float lateral, int &screenX,
                        int &screenY, int &scale) {
  if (distance < RACER_RECYCLE_DISTANCE || distance > RACER_FAR_DISTANCE) {
    return false;
  }

  const float proximity =
      racerClamp(1.0f - distance / RACER_FAR_DISTANCE, 0.0f, 1.0f);
  screenY = RACER_HORIZON_Y + static_cast<int>(proximity * proximity *
                                               (SCREEN_H - RACER_HORIZON_Y));

  float ignoredDepth;
  int roadCenter;
  int roadHalfWidth;
  racerRoadGeometry(constrain(screenY, RACER_HORIZON_Y, SCREEN_H - 1),
                    ignoredDepth, roadCenter, roadHalfWidth);
  screenX = roadCenter + static_cast<int>(lateral * roadHalfWidth * 0.78f);
  scale = constrain(2 + static_cast<int>(proximity * 16.0f), 2, 18);
  return true;
}

void racerDrawPlayerCar() {
  const int centerX = SCREEN_W / 2;
  const int steering =
      racerPlayerLateral > 0.05f ? 2 : (racerPlayerLateral < -0.05f ? -2 : 0);
  const int x = centerX - 11 + steering;
  const int y = 101;

  tft.fillRoundRect(x, y + 5, 22, 20, 3, ST77XX_RED);
  tft.fillTriangle(x + 3, y + 8, x + 8, y, x + 11, y + 8, 0x7DFF);
  tft.fillTriangle(x + 19, y + 8, x + 14, y, x + 11, y + 8, 0x7DFF);
  tft.fillRect(x - 2, y + 15, 4, 8, ST77XX_BLACK);
  tft.fillRect(x + 20, y + 15, 4, 8, ST77XX_BLACK);
  tft.fillRect(x + 5, y + 18, 12, 4, ST77XX_YELLOW);
}

void racerDrawHud() {
  tft.fillRect(0, 0, SCREEN_W, 20, ST77XX_BLACK);
  tft.setTextSize(1);

  tft.setTextColor(racerDurability <= 25 ? ST77XX_RED : ST77XX_GREEN);
  tft.setCursor(1, 1);
  tft.print("HP");
  tft.print(racerDurability);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(46, 1);
  tft.print("SPD");
  tft.print(static_cast<int>(racerSpeed));

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(108, 1);
  tft.print("T");
  tft.print(static_cast<int>(racerTurbo));

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(1, 11);
  tft.print("SC");
  tft.print(racerScore);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(82, 11);
  tft.print("HI");
  tft.print(racerHighScore);
}

bool racerTrafficSpacingClear(int8_t lane, float distance, int ignoredIndex) {
  const float lateral = racerLanePosition(lane);

  for (uint8_t index = 0; index < RACER_TRAFFIC_CAPACITY; ++index) {
    if ((ignoredIndex >= 0 && index == static_cast<uint8_t>(ignoredIndex)) ||
        !racerTraffic[index].active) {
      continue;
    }

    if (fabsf(racerTraffic[index].lateral - lateral) < 0.20f &&
        fabsf(racerTraffic[index].distance - distance) < 18.0f) {
      return false;
    }
  }

  return true;
}

bool racerSpawnTraffic(uint8_t index, float minimumDistance,
                       unsigned long now) {
  const int milestone = min(3, static_cast<int>(racerWorldDistance / 750.0f));
  const int minimumSpeed = 30 - milestone * 3;
  const int maximumSpeed = 46 + milestone * 3;
  const int8_t firstLane = static_cast<int8_t>(random(-1, 2));

  for (uint8_t attempt = 0; attempt < 3; ++attempt) {
    const int8_t lane =
        static_cast<int8_t>(((firstLane + 1 + attempt) % 3) - 1);
    const float distance =
        fmaxf(minimumDistance, static_cast<float>(random(80, 141)));

    if (!racerTrafficSpacingClear(lane, distance, index)) {
      continue;
    }

    const uint16_t colors[] = {
        ST77XX_RED,     ST77XX_BLUE,  ST77XX_YELLOW,
        ST77XX_MAGENTA, ST77XX_GREEN, ST77XX_ORANGE,
    };

    racerTraffic[index] = {
        racerLanePosition(lane),
        distance,
        static_cast<float>(random(minimumSpeed, maximumSpeed + 1)),
        colors[index % RACER_TRAFFIC_CAPACITY],
        true,
        true,
    };
    racerTrafficRetryAt[index] = 0;
    return true;
  }

  racerTraffic[index].active = false;
  racerTrafficRetryAt[index] = now + 1000;
  return false;
}

void racerResetEntities(unsigned long now) {
  for (uint8_t index = 0; index < RACER_TRAFFIC_CAPACITY; ++index) {
    racerTraffic[index].active = false;
    racerTrafficRetryAt[index] = 0;
  }

  for (uint8_t index = 0; index < RACER_PICKUP_CAPACITY; ++index) {
    racerPickups[index] = {0.0f, 0.0f, false};
  }

  racerActiveTraffic = 3;
  for (uint8_t index = 0; index < racerActiveTraffic; ++index) {
    racerSpawnTraffic(index, 82.0f + index * 22.0f, now);
  }

  racerNextPickupAt = now + random(8000, 12001);
}

void racerUpdateTraffic(float deltaSeconds, unsigned long now) {
  const uint8_t desiredTraffic = static_cast<uint8_t>(
      min(static_cast<int>(RACER_TRAFFIC_CAPACITY),
          3 + static_cast<int>(racerWorldDistance / 750.0f)));

  while (racerActiveTraffic < desiredTraffic) {
    const uint8_t index = racerActiveTraffic++;
    racerSpawnTraffic(index, 100.0f, now);
  }

  for (uint8_t index = 0; index < racerActiveTraffic; ++index) {
    RacerTraffic &car = racerTraffic[index];

    if (!car.active) {
      if (racerTrafficRetryAt[index] == 0 ||
          static_cast<long>(now - racerTrafficRetryAt[index]) >= 0) {
        racerSpawnTraffic(index, 90.0f, now);
      }
      continue;
    }

    car.distance -= (racerSpeed - car.speed) * deltaSeconds * 0.60f;

    const bool overlaps = fabsf(car.lateral - racerPlayerLateral) < 0.27f;
    if (fabsf(car.distance) <= RACER_COLLISION_DISTANCE && overlaps &&
        static_cast<long>(now - racerCollisionSafeUntil) >= 0) {
      racerDurability = max(0, racerDurability - 25);
      racerSpeed *= 0.45f;
      racerCollisionSafeUntil = now + 1200;
      car.cleanPass = false;
      car.distance = RACER_RECYCLE_DISTANCE - 1.0f;
      tone(BUZZER_PIN, 230, 130);
      vibrateFor(100);
    }

    if (car.distance < RACER_RECYCLE_DISTANCE) {
      if (car.cleanPass) {
        racerOvertakeScore += 100;
        racerScore += 100;
        tone(BUZZER_PIN, 1100, 45);
      }
      racerSpawnTraffic(index, 90.0f, now);
    } else if (car.distance > RACER_FAR_DISTANCE + 20.0f) {
      racerSpawnTraffic(index, 90.0f, now);
    }
  }
}

bool racerSpawnPickup(unsigned long now) {
  for (uint8_t index = 0; index < RACER_PICKUP_CAPACITY; ++index) {
    if (racerPickups[index].active) {
      continue;
    }

    const int8_t firstLane = static_cast<int8_t>(random(-1, 2));

    for (uint8_t attempt = 0; attempt < 3; ++attempt) {
      const int8_t lane =
          static_cast<int8_t>(((firstLane + 1 + attempt) % 3) - 1);
      const float distance = static_cast<float>(random(90, 131));

      if (!racerTrafficSpacingClear(lane, distance, -1)) {
        continue;
      }

      racerPickups[index] = {
          racerLanePosition(lane),
          distance,
          true,
      };
      racerNextPickupAt = now + random(8000, 12001);
      return true;
    }
    break;
  }

  racerNextPickupAt = now + 1000;
  return false;
}

void racerUpdatePickups(float deltaSeconds, unsigned long now) {
  if (static_cast<long>(now - racerNextPickupAt) >= 0) {
    racerSpawnPickup(now);
  }

  for (uint8_t index = 0; index < RACER_PICKUP_CAPACITY; ++index) {
    RacerPickup &pickup = racerPickups[index];
    if (!pickup.active) {
      continue;
    }

    pickup.distance -= racerSpeed * deltaSeconds * 0.60f;

    if (fabsf(pickup.distance) <= 5.0f &&
        fabsf(pickup.lateral - racerPlayerLateral) < 0.28f) {
      racerTurbo = fminf(100.0f, racerTurbo + 30.0f);
      pickup.active = false;
      racerNextPickupAt = now + random(8000, 12001);
      tone(BUZZER_PIN, 1700, 80);
      vibrateFor(40);
    } else if (pickup.distance < RACER_RECYCLE_DISTANCE) {
      pickup.active = false;
      racerNextPickupAt = now + 1000;
    }
  }
}

void racerBuildSpriteList() {
  racerSpriteCount = 0;

  for (uint8_t index = 0; index < racerActiveTraffic; ++index) {
    if (racerTraffic[index].active) {
      racerSprites[racerSpriteCount++] = {
          racerTraffic[index].distance,
          false,
          index,
      };
    }
  }

  for (uint8_t index = 0; index < RACER_PICKUP_CAPACITY; ++index) {
    if (racerPickups[index].active) {
      racerSprites[racerSpriteCount++] = {
          racerPickups[index].distance,
          true,
          index,
      };
    }
  }

  for (uint8_t index = 1; index < racerSpriteCount; ++index) {
    const RacerSpriteRef key = racerSprites[index];
    int position = index - 1;

    while (position >= 0 && racerSprites[position].distance < key.distance) {
      racerSprites[position + 1] = racerSprites[position];
      --position;
    }

    racerSprites[position + 1] = key;
  }
}

void racerRenderEntities() {
  racerBuildSpriteList();

  for (uint8_t spriteIndex = 0; spriteIndex < racerSpriteCount; ++spriteIndex) {
    const RacerSpriteRef &sprite = racerSprites[spriteIndex];
    const float lateral = sprite.pickup ? racerPickups[sprite.index].lateral
                                        : racerTraffic[sprite.index].lateral;
    int x;
    int y;
    int scale;

    if (!racerProjectEntity(sprite.distance, lateral, x, y, scale)) {
      continue;
    }

    if (sprite.pickup) {
      const int width = max(3, scale * 2 / 3);
      const int height = max(4, scale);
      tft.fillRect(x - width / 2, y - height, width, height, ST77XX_CYAN);
      tft.drawRect(x - width / 2, y - height, width, height, ST77XX_WHITE);
      continue;
    }

    const int width = max(4, scale);
    const int height = max(5, scale * 4 / 3);
    const uint16_t color = racerTraffic[sprite.index].color;

    tft.fillRect(x - width / 2, y - height, width, height, color);
    tft.fillRect(x - width / 3, y - height + 2, max(2, width * 2 / 3),
                 max(2, height / 3), 0x7DFF);
    tft.fillRect(x - width / 2, y - 2, max(2, width / 4), 2, ST77XX_BLACK);
    tft.fillRect(x + width / 4, y - 2, max(2, width / 4), 2, ST77XX_BLACK);
  }
}

void racerRenderFrame() {
  racerRenderRoad();
  racerRenderEntities();
  racerDrawPlayerCar();
  racerDrawHud();
}

void racerDrawPauseOverlay() {
  tft.fillRect(52, 45, 56, 20, ST77XX_BLACK);
  tft.drawRect(52, 45, 56, 20, ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(62, 51);
  tft.print("PAUSED");
}

void racerDrawGameOver() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(25, 16);
  tft.print("GAME OVER");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(46, 52);
  tft.print("Score: ");
  tft.print(racerScore);
  tft.setCursor(34, 68);
  tft.print("Racer High: ");
  tft.print(racerHighScore);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(35, 94);
  tft.print("RIGHT: RESTART");
  tft.setCursor(47, 108);
  tft.print("LEFT: MENU");
}

void endRacerGame() {
  if (appState != RACER_PLAYING) {
    return;
  }

  if (racerScore > racerHighScore) {
    racerHighScore = static_cast<unsigned int>(racerScore);
    prefs.putUInt("raceHigh", racerHighScore);
  }

  appState = RACER_GAME_OVER;
  beepGameOver();
  vibrateFor(300);
  racerDrawGameOver();
}

void resetRacer() {
  const unsigned long now = millis();
  racerPlayerLateral = 0.0f;
  racerSpeed = 25.0f;
  racerWorldDistance = 0.0f;
  racerTurbo = 50.0f;
  racerDurability = 100;
  racerOvertakeScore = 0;
  racerScore = 0;
  racerPaused = false;
  racerTurboActive = false;
  racerTurboReady = false;
  racerCollisionSafeUntil = now;
  racerLastFrame = now;
  racerPauseStartedAt = 0;
  racerResetEntities(now);

  appState = RACER_PLAYING;
  tft.fillScreen(ST77XX_BLACK);
  racerRenderFrame();
}

void updateRacer(const InputFrame &input) {
  const unsigned long now = millis();

  if (input.leftPressed) {
    racerPaused = !racerPaused;

    if (racerPaused) {
      racerPauseStartedAt = now;
      racerDrawPauseOverlay();
    } else {
      const unsigned long pausedFor = now - racerPauseStartedAt;
      racerLastFrame = now;
      racerCollisionSafeUntil += pausedFor;
      racerNextPickupAt += pausedFor;
      racerPauseStartedAt = 0;
      racerRenderFrame();
    }
  }

  if (racerPaused) {
    return;
  }

  if (now - racerLastFrame < RACER_FRAME_MS) {
    return;
  }

  const float deltaSeconds =
      min(static_cast<unsigned long>(100), now - racerLastFrame) / 1000.0f;
  racerLastFrame = now;

  racerUpdateDriving(input, deltaSeconds);
  racerUpdateTraffic(deltaSeconds, now);
  racerUpdatePickups(deltaSeconds, now);

  if (racerDurability <= 0) {
    endRacerGame();
    return;
  }

  racerRenderFrame();
}

void updateRacerGameOver(const InputFrame &input) {
  if (input.rightPressed) {
    resetRacer();
  } else if (input.leftPressed) {
    enterMenu();
  }
}

//==================================================
// PAC-MAZE
//==================================================
constexpr uint8_t PAC_COLS = 20;
constexpr uint8_t PAC_ROWS = 15;
constexpr uint8_t PAC_TILE = 8;
constexpr uint8_t PAC_HUD_H = 8;
constexpr uint8_t PAC_TUNNEL_ROW = 7;
constexpr unsigned long PAC_FRAME_MS = 40;

enum PacTile : uint8_t {
  PAC_EMPTY,
  PAC_WALL,
  PAC_PELLET,
  PAC_POWER,
  PAC_HOUSE,
  PAC_DOOR
};

enum PacDirection : uint8_t {
  PAC_DIR_UP,
  PAC_DIR_DOWN,
  PAC_DIR_LEFT,
  PAC_DIR_RIGHT,
  PAC_DIR_NONE
};

enum PacGhostMode : uint8_t {
  PAC_GHOST_HOUSE,
  PAC_GHOST_SCATTER,
  PAC_GHOST_CHASE,
  PAC_GHOST_FRIGHTENED,
  PAC_GHOST_EATEN
};

struct PacActor {
  int16_t x;
  int16_t y;
  PacDirection direction;
};

struct PacGhost {
  PacActor actor;
  int8_t homeCol;
  int8_t homeRow;
  int8_t scatterCol;
  int8_t scatterRow;
  PacGhostMode mode;
  unsigned long releaseAt;
  float stepAccumulator;
  uint16_t color;
};

// Explicit prototypes keep Arduino's sketch preprocessor from emitting
// custom-type prototypes before the Pac-Maze type declarations above.
int pacDirectionX(PacDirection direction);
int pacDirectionY(PacDirection direction);
PacDirection pacOpposite(PacDirection direction);
bool pacAtTileCenter(const PacActor &actor);
void pacActorTile(const PacActor &actor, int &col, int &row);
bool pacDirectionOpen(const PacActor &actor, PacDirection direction,
                      bool ghost);
bool pacMoveActor(PacActor &actor, PacDirection direction, bool ghost);
PacDirection pacInputDirection(const InputFrame &input);
PacDirection pacChooseGhostDirection(uint8_t index);
void pacDrawActorGhost(uint8_t index, unsigned long now);

const uint8_t PAC_MAP[PAC_ROWS][PAC_COLS] PROGMEM = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 3, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 3, 1},
    {1, 2, 1, 1, 2, 1, 2, 1, 1, 2, 2, 1, 1, 2, 1, 2, 1, 1, 2, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
    {1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 1, 4, 4, 1, 2, 2, 2, 2, 2, 2, 2, 1},
    {1, 1, 1, 2, 1, 1, 2, 2, 1, 4, 4, 1, 2, 2, 1, 1, 2, 1, 1, 1},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 1, 2, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 1, 1, 2, 1, 1, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
    {1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 1},
    {1, 2, 1, 1, 2, 1, 2, 1, 1, 2, 2, 1, 1, 2, 1, 2, 1, 1, 2, 1},
    {1, 3, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 3, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

uint8_t pacCollectibles[PAC_ROWS][PAC_COLS];
PacActor pacPlayer;
PacGhost pacGhosts[4];
PacDirection pacRequested = PAC_DIR_NONE;
float pacPlayerAccumulator = 0.0f;
unsigned long pacScore = 0;
uint8_t pacLives = 3;
uint16_t pacLevel = 1;
uint16_t pacPelletsRemaining = 0;
uint16_t pacPelletsEaten = 0;
uint8_t pacGhostCombo = 0;
bool pacPaused = false;
bool pacScatterMode = true;
uint8_t pacTransition = 0;
unsigned long pacLastFrame = 0;
unsigned long pacModeDeadline = 0;
unsigned long pacFrightenedUntil = 0;
unsigned long pacProtectedUntil = 0;
unsigned long pacTransitionUntil = 0;
unsigned long pacPauseStartedAt = 0;

uint8_t pacTileAt(int col, int row) {
  if (row == PAC_TUNNEL_ROW && (col < 0 || col >= PAC_COLS)) {
    return PAC_EMPTY;
  }
  if (col < 0 || col >= PAC_COLS || row < 0 || row >= PAC_ROWS) {
    return PAC_WALL;
  }
  return pgm_read_byte(&PAC_MAP[row][col]);
}

bool pacWalkable(int col, int row, bool ghost) {
  const uint8_t tile = pacTileAt(col, row);
  if (tile == PAC_WALL)
    return false;
  return ghost || (tile != PAC_HOUSE && tile != PAC_DOOR);
}

int pacDirectionX(PacDirection direction) {
  return direction == PAC_DIR_LEFT ? -1 : direction == PAC_DIR_RIGHT ? 1 : 0;
}

int pacDirectionY(PacDirection direction) {
  return direction == PAC_DIR_UP ? -1 : direction == PAC_DIR_DOWN ? 1 : 0;
}

PacDirection pacOpposite(PacDirection direction) {
  switch (direction) {
  case PAC_DIR_UP:
    return PAC_DIR_DOWN;
  case PAC_DIR_DOWN:
    return PAC_DIR_UP;
  case PAC_DIR_LEFT:
    return PAC_DIR_RIGHT;
  case PAC_DIR_RIGHT:
    return PAC_DIR_LEFT;
  default:
    return PAC_DIR_NONE;
  }
}

bool pacAtTileCenter(const PacActor &actor) {
  if (actor.x < 0 || actor.x >= SCREEN_W)
    return false;
  return actor.x % PAC_TILE == PAC_TILE / 2 &&
         (actor.y - PAC_HUD_H) % PAC_TILE == PAC_TILE / 2;
}

void pacActorTile(const PacActor &actor, int &col, int &row) {
  col = constrain(actor.x / PAC_TILE, 0, PAC_COLS - 1);
  row = constrain((actor.y - PAC_HUD_H) / PAC_TILE, 0, PAC_ROWS - 1);
}

bool pacDirectionOpen(const PacActor &actor, PacDirection direction,
                      bool ghost) {
  if (direction == PAC_DIR_NONE)
    return false;
  int col;
  int row;
  pacActorTile(actor, col, row);
  return pacWalkable(col + pacDirectionX(direction),
                     row + pacDirectionY(direction), ghost);
}

bool pacMoveActor(PacActor &actor, PacDirection direction, bool ghost) {
  if (direction == PAC_DIR_NONE)
    return false;
  if (pacAtTileCenter(actor) && !pacDirectionOpen(actor, direction, ghost)) {
    return false;
  }
  if (direction == PAC_DIR_UP || direction == PAC_DIR_DOWN) {
    actor.x = actor.x / PAC_TILE * PAC_TILE + PAC_TILE / 2;
  } else {
    actor.y =
        (actor.y - PAC_HUD_H) / PAC_TILE * PAC_TILE + PAC_HUD_H + PAC_TILE / 2;
  }
  actor.x += pacDirectionX(direction);
  actor.y += pacDirectionY(direction);
  actor.direction = direction;
  const int row = (actor.y - PAC_HUD_H) / PAC_TILE;
  if (row == PAC_TUNNEL_ROW) {
    if (actor.x < -PAC_TILE / 2) {
      actor.x = SCREEN_W + PAC_TILE / 2;
    } else if (actor.x > SCREEN_W + PAC_TILE / 2) {
      actor.x = -PAC_TILE / 2;
    }
  }
  return true;
}

PacDirection pacInputDirection(const InputFrame &input) {
  if (input.left)
    return PAC_DIR_LEFT;
  if (input.right)
    return PAC_DIR_RIGHT;
  if (input.up)
    return PAC_DIR_UP;
  if (input.down)
    return PAC_DIR_DOWN;
  return PAC_DIR_NONE;
}

void pacResetCollectibles() {
  pacPelletsRemaining = 0;
  for (uint8_t row = 0; row < PAC_ROWS; ++row) {
    for (uint8_t col = 0; col < PAC_COLS; ++col) {
      const uint8_t tile = pacTileAt(col, row);
      pacCollectibles[row][col] =
          tile == PAC_PELLET || tile == PAC_POWER ? tile : 0;
      if (pacCollectibles[row][col] != 0) {
        ++pacPelletsRemaining;
      }
    }
  }
}

void pacResetActors(unsigned long now) {
  pacPlayer = {84, 100, PAC_DIR_LEFT};
  pacRequested = PAC_DIR_LEFT;
  pacPlayerAccumulator = 0.0f;
  const int8_t homeCols[4] = {9, 10, 9, 10};
  const int8_t homeRows[4] = {6, 6, 5, 5};
  const int8_t scatterCols[4] = {18, 1, 18, 1};
  const int8_t scatterRows[4] = {1, 1, 13, 13};
  const uint16_t colors[4] = {ST77XX_RED, ST77XX_MAGENTA, ST77XX_CYAN,
                              ST77XX_ORANGE};
  for (uint8_t index = 0; index < 4; ++index) {
    pacGhosts[index] = {
        {static_cast<int16_t>(homeCols[index] * 8 + 4),
         static_cast<int16_t>(PAC_HUD_H + homeRows[index] * 8 + 4), PAC_DIR_UP},
        homeCols[index],
        homeRows[index],
        scatterCols[index],
        scatterRows[index],
        PAC_GHOST_HOUSE,
        now + index * 2000UL,
        0.0f,
        colors[index]};
  }
}

void pacStartFrightened(unsigned long now) {
  pacFrightenedUntil = now + 6000;
  pacModeDeadline += 6000;
  pacGhostCombo = 0;
  for (uint8_t index = 0; index < 4; ++index) {
    PacGhost &ghost = pacGhosts[index];
    if (ghost.mode == PAC_GHOST_SCATTER || ghost.mode == PAC_GHOST_CHASE ||
        ghost.mode == PAC_GHOST_FRIGHTENED) {
      ghost.mode = PAC_GHOST_FRIGHTENED;
      ghost.actor.direction = pacOpposite(ghost.actor.direction);
    }
  }
  tone(BUZZER_PIN, 1500, 100);
  vibrateFor(40);
}

void pacConsumeCollectible(unsigned long now) {
  int col;
  int row;
  pacActorTile(pacPlayer, col, row);
  const uint8_t item = pacCollectibles[row][col];
  if (item == 0)
    return;
  pacCollectibles[row][col] = 0;
  --pacPelletsRemaining;
  ++pacPelletsEaten;
  if (item == PAC_POWER) {
    pacScore += 50;
    pacStartFrightened(now);
  } else {
    pacScore += 10;
    if ((pacPelletsEaten & 3) == 0) {
      tone(BUZZER_PIN, 950, 18);
    }
  }
}

void pacUpdatePlayer(const InputFrame &input, unsigned long now) {
  const PacDirection requested = pacInputDirection(input);
  if (requested != PAC_DIR_NONE)
    pacRequested = requested;
  const float speed = min(27, 24 + static_cast<int>(pacLevel));
  pacPlayerAccumulator += speed * PAC_FRAME_MS / 1000.0f;
  while (pacPlayerAccumulator >= 1.0f) {
    pacPlayerAccumulator -= 1.0f;
    if (pacAtTileCenter(pacPlayer) &&
        pacDirectionOpen(pacPlayer, pacRequested, false)) {
      pacPlayer.direction = pacRequested;
    }
    pacMoveActor(pacPlayer, pacPlayer.direction, false);
    pacConsumeCollectible(now);
  }
}

void pacGhostTarget(uint8_t index, int &targetCol, int &targetRow) {
  PacGhost &ghost = pacGhosts[index];
  int playerCol;
  int playerRow;
  pacActorTile(pacPlayer, playerCol, playerRow);
  if (ghost.mode == PAC_GHOST_EATEN) {
    targetCol = ghost.homeCol;
    targetRow = ghost.homeRow;
  } else if (pacScatterMode) {
    targetCol = ghost.scatterCol;
    targetRow = ghost.scatterRow;
  } else if (index == 0) {
    targetCol = playerCol;
    targetRow = playerRow;
  } else if (index == 1) {
    targetCol = playerCol + pacDirectionX(pacPlayer.direction) * 4;
    targetRow = playerRow + pacDirectionY(pacPlayer.direction) * 4;
  } else if (index == 2) {
    int redCol;
    int redRow;
    pacActorTile(pacGhosts[0].actor, redCol, redRow);
    const int aheadCol = playerCol + pacDirectionX(pacPlayer.direction) * 2;
    const int aheadRow = playerRow + pacDirectionY(pacPlayer.direction) * 2;
    targetCol = aheadCol * 2 - redCol;
    targetRow = aheadRow * 2 - redRow;
  } else {
    int ghostCol;
    int ghostRow;
    pacActorTile(ghost.actor, ghostCol, ghostRow);
    const int distance = abs(ghostCol - playerCol) + abs(ghostRow - playerRow);
    targetCol = distance > 6 ? playerCol : ghost.scatterCol;
    targetRow = distance > 6 ? playerRow : ghost.scatterRow;
  }
  targetCol = constrain(targetCol, 0, PAC_COLS - 1);
  targetRow = constrain(targetRow, 0, PAC_ROWS - 1);
}

PacDirection pacChooseGhostDirection(uint8_t index) {
  PacGhost &ghost = pacGhosts[index];
  const PacDirection order[4] = {PAC_DIR_UP, PAC_DIR_LEFT, PAC_DIR_DOWN,
                                 PAC_DIR_RIGHT};
  PacDirection legal[4];
  uint8_t legalCount = 0;
  for (uint8_t choice = 0; choice < 4; ++choice) {
    if (pacDirectionOpen(ghost.actor, order[choice], true)) {
      legal[legalCount++] = order[choice];
    }
  }
  if (legalCount == 0)
    return PAC_DIR_NONE;
  if (legalCount > 1) {
    const PacDirection reverse = pacOpposite(ghost.actor.direction);
    for (uint8_t i = 0; i < legalCount; ++i) {
      if (legal[i] == reverse) {
        for (uint8_t j = i; j + 1 < legalCount; ++j) {
          legal[j] = legal[j + 1];
        }
        --legalCount;
        break;
      }
    }
  }
  if (ghost.mode == PAC_GHOST_FRIGHTENED) {
    return legal[random(legalCount)];
  }
  int targetCol;
  int targetRow;
  pacGhostTarget(index, targetCol, targetRow);
  int actorCol;
  int actorRow;
  pacActorTile(ghost.actor, actorCol, actorRow);
  PacDirection best = legal[0];
  int bestDistance = 1000;
  for (uint8_t i = 0; i < legalCount; ++i) {
    const int nextCol = actorCol + pacDirectionX(legal[i]);
    const int nextRow = actorRow + pacDirectionY(legal[i]);
    const int distance = abs(nextCol - targetCol) + abs(nextRow - targetRow);
    if (distance < bestDistance) {
      bestDistance = distance;
      best = legal[i];
    }
  }
  return best;
}

void pacUpdateGhosts(unsigned long now) {
  if (pacFrightenedUntil != 0 &&
      static_cast<long>(now - pacFrightenedUntil) >= 0) {
    pacFrightenedUntil = 0;
    for (uint8_t index = 0; index < 4; ++index) {
      if (pacGhosts[index].mode == PAC_GHOST_FRIGHTENED) {
        pacGhosts[index].mode =
            pacScatterMode ? PAC_GHOST_SCATTER : PAC_GHOST_CHASE;
      }
    }
  }
  if (static_cast<long>(now - pacModeDeadline) >= 0) {
    pacScatterMode = !pacScatterMode;
    pacModeDeadline = now + (pacScatterMode ? 7000 : 20000);
  }
  for (uint8_t index = 0; index < 4; ++index) {
    PacGhost &ghost = pacGhosts[index];
    if (ghost.mode == PAC_GHOST_HOUSE) {
      if (static_cast<long>(now - ghost.releaseAt) < 0)
        continue;
      ghost.actor = {84, 68, PAC_DIR_LEFT};
      ghost.mode = pacFrightenedUntil != 0
                       ? PAC_GHOST_FRIGHTENED
                       : (pacScatterMode ? PAC_GHOST_SCATTER : PAC_GHOST_CHASE);
    }
    const float normalSpeed = min(30, 21 + static_cast<int>(pacLevel));
    const float speed =
        ghost.mode == PAC_GHOST_EATEN
            ? 34.0f
            : (ghost.mode == PAC_GHOST_FRIGHTENED ? normalSpeed * 0.70f
                                                  : normalSpeed);
    ghost.stepAccumulator += speed * PAC_FRAME_MS / 1000.0f;
    while (ghost.stepAccumulator >= 1.0f) {
      ghost.stepAccumulator -= 1.0f;
      if (pacAtTileCenter(ghost.actor)) {
        ghost.actor.direction = pacChooseGhostDirection(index);
      }
      pacMoveActor(ghost.actor, ghost.actor.direction, true);
      int col;
      int row;
      pacActorTile(ghost.actor, col, row);
      if (ghost.mode == PAC_GHOST_EATEN && col == ghost.homeCol &&
          row == ghost.homeRow) {
        ghost.mode = PAC_GHOST_HOUSE;
        ghost.releaseAt = now + 1000;
        break;
      }
    }
  }
}

void pacResolveCollisions(unsigned long now) {
  for (uint8_t index = 0; index < 4; ++index) {
    PacGhost &ghost = pacGhosts[index];
    if (ghost.mode == PAC_GHOST_HOUSE || ghost.mode == PAC_GHOST_EATEN)
      continue;
    const long dx = ghost.actor.x - pacPlayer.x;
    const long dy = ghost.actor.y - pacPlayer.y;
    if (dx * dx + dy * dy > 25)
      continue;
    if (ghost.mode == PAC_GHOST_FRIGHTENED) {
      pacScore += 200UL << min(pacGhostCombo, static_cast<uint8_t>(3));
      ++pacGhostCombo;
      ghost.mode = PAC_GHOST_EATEN;
      tone(BUZZER_PIN, 1750, 90);
      vibrateFor(60);
    } else if (static_cast<long>(now - pacProtectedUntil) >= 0) {
      --pacLives;
      pacTransition = 1;
      pacTransitionUntil = now + 1200;
      tone(BUZZER_PIN, 260, 200);
      vibrateFor(200);
      return;
    }
  }
}

void pacDrawActorGhost(uint8_t index, unsigned long now) {
  const PacGhost &ghost = pacGhosts[index];
  uint16_t color = ghost.color;
  if (ghost.mode == PAC_GHOST_FRIGHTENED) {
    const bool ending = pacFrightenedUntil - now <= 2000;
    color = ending && ((now / 200) & 1) ? ST77XX_WHITE : ST77XX_BLUE;
  }
  const int x = ghost.actor.x;
  const int y = ghost.actor.y;
  if (ghost.mode != PAC_GHOST_EATEN) {
    tft.fillCircle(x, y - 1, 4, color);
    tft.fillRect(x - 4, y - 1, 8, 5, color);
  }
  tft.fillCircle(x - 2, y - 1, 1, ST77XX_WHITE);
  tft.fillCircle(x + 2, y - 1, 1, ST77XX_WHITE);
}

void pacRenderFrame() {
  const unsigned long now = millis();
  tft.fillScreen(ST77XX_BLACK);
  for (uint8_t row = 0; row < PAC_ROWS; ++row) {
    for (uint8_t col = 0; col < PAC_COLS; ++col) {
      const int x = col * PAC_TILE;
      const int y = PAC_HUD_H + row * PAC_TILE;
      if (pacTileAt(col, row) == PAC_WALL) {
        tft.drawRect(x + 1, y + 1, 6, 6, ST77XX_BLUE);
      } else if (pacCollectibles[row][col] == PAC_PELLET) {
        tft.fillRect(x + 3, y + 3, 2, 2, ST77XX_WHITE);
      } else if (pacCollectibles[row][col] == PAC_POWER && ((now / 250) & 1)) {
        tft.fillCircle(x + 4, y + 4, 3, ST77XX_WHITE);
      }
    }
  }
  tft.fillCircle(pacPlayer.x, pacPlayer.y, 4, ST77XX_YELLOW);
  tft.fillTriangle(pacPlayer.x, pacPlayer.y,
                   pacPlayer.x + pacDirectionX(pacPlayer.direction) * 5 +
                       pacDirectionY(pacPlayer.direction) * 3,
                   pacPlayer.y + pacDirectionY(pacPlayer.direction) * 5 +
                       pacDirectionX(pacPlayer.direction) * 3,
                   pacPlayer.x + pacDirectionX(pacPlayer.direction) * 5 -
                       pacDirectionY(pacPlayer.direction) * 3,
                   pacPlayer.y + pacDirectionY(pacPlayer.direction) * 5 -
                       pacDirectionX(pacPlayer.direction) * 3,
                   ST77XX_BLACK);
  for (uint8_t index = 0; index < 4; ++index) {
    pacDrawActorGhost(index, now);
  }
  tft.fillRect(0, 0, SCREEN_W, PAC_HUD_H, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(0, 0);
  tft.print("S");
  tft.print(pacScore);
  tft.setCursor(76, 0);
  tft.print("L");
  tft.print(pacLives);
  tft.setCursor(112, 0);
  tft.print("LV");
  tft.print(pacLevel);
}

void pacResetLevel(unsigned long now) {
  pacResetCollectibles();
  pacResetActors(now);
  pacScatterMode = true;
  pacModeDeadline = now + 7000;
  pacFrightenedUntil = 0;
  pacGhostCombo = 0;
  pacTransition = 0;
}

void pacDrawPauseOverlay() {
  tft.fillRect(52, 52, 56, 20, ST77XX_BLACK);
  tft.drawRect(52, 52, 56, 20, ST77XX_YELLOW);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(62, 58);
  tft.print("PAUSED");
}

void pacDrawGameOver() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(25, 16);
  tft.print("GAME OVER");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(42, 52);
  tft.print("Score: ");
  tft.print(pacScore);
  tft.setCursor(35, 68);
  tft.print("Pac High: ");
  tft.print(pacHighScore);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(35, 94);
  tft.print("RIGHT: RESTART");
  tft.setCursor(47, 108);
  tft.print("LEFT: MENU");
}

void endPacGame() {
  if (appState != PAC_PLAYING)
    return;
  if (pacScore > pacHighScore) {
    pacHighScore = static_cast<unsigned int>(pacScore);
    prefs.putUInt("pacHigh", pacHighScore);
  }
  appState = PAC_GAME_OVER;
  beepGameOver();
  vibrateFor(300);
  pacDrawGameOver();
}

void resetPac() {
  const unsigned long now = millis();
  pacScore = 0;
  pacLives = 3;
  pacLevel = 1;
  pacPelletsEaten = 0;
  pacPaused = false;
  pacProtectedUntil = now + 1000;
  pacLastFrame = now;
  pacPauseStartedAt = 0;
  pacResetLevel(now);
  appState = PAC_PLAYING;
  pacRenderFrame();
}

void updatePac(const InputFrame &input) {
  const unsigned long now = millis();
  if (input.leftPressed) {
    pacPaused = !pacPaused;
    if (pacPaused) {
      pacPauseStartedAt = now;
      pacDrawPauseOverlay();
    } else {
      const unsigned long pausedFor = now - pacPauseStartedAt;
      pacModeDeadline += pausedFor;
      if (pacFrightenedUntil)
        pacFrightenedUntil += pausedFor;
      pacProtectedUntil += pausedFor;
      if (pacTransitionUntil)
        pacTransitionUntil += pausedFor;
      for (uint8_t i = 0; i < 4; ++i) {
        pacGhosts[i].releaseAt += pausedFor;
      }
      pacLastFrame = now;
      pacRenderFrame();
    }
  }
  if (pacPaused)
    return;
  if (pacTransition != 0) {
    if (static_cast<long>(now - pacTransitionUntil) < 0)
      return;
    if (pacTransition == 1) {
      if (pacLives == 0) {
        endPacGame();
        return;
      }
      pacResetActors(now);
      pacProtectedUntil = now + 1500;
    } else {
      ++pacLevel;
      pacResetLevel(now);
    }
    pacTransition = 0;
    pacLastFrame = now;
  }
  if (now - pacLastFrame < PAC_FRAME_MS)
    return;
  pacLastFrame = now;
  pacUpdatePlayer(input, now);
  pacUpdateGhosts(now);
  pacResolveCollisions(now);
  if (pacPelletsRemaining == 0 && pacTransition == 0) {
    pacScore += 500;
    pacTransition = 2;
    pacTransitionUntil = now + 1000;
    tone(BUZZER_PIN, 1300, 90);
  }
  pacRenderFrame();
}

void updatePacGameOver(const InputFrame &input) {
  if (input.rightPressed)
    resetPac();
  else if (input.leftPressed)
    enterMenu();
}

//==================================================
// SPACE RAID
//==================================================
constexpr uint8_t SPACE_INVADER_ROWS = 5;
constexpr uint8_t SPACE_INVADER_COLS = 8;
constexpr uint8_t SPACE_PLAYER_SHOTS = 3;
constexpr uint8_t SPACE_ENEMY_SHOTS = 6;
constexpr uint8_t SPACE_SHIELD_COUNT = 3;
constexpr uint8_t SPACE_SHIELD_COLS = 8;
constexpr uint8_t SPACE_SHIELD_ROWS = 5;
constexpr uint8_t SPACE_INVADER_W = 12;
constexpr uint8_t SPACE_INVADER_H = 8;
constexpr uint8_t SPACE_INVADER_X_STEP = 16;
constexpr uint8_t SPACE_INVADER_Y_STEP = 10;
constexpr unsigned long SPACE_FRAME_MS = 25;

struct SpaceShot {
  int16_t x;
  int16_t y;
  bool active;
};

struct SpaceBonus {
  int16_t x;
  int8_t direction;
  bool active;
};

bool spaceInvaders[SPACE_INVADER_ROWS][SPACE_INVADER_COLS];
SpaceShot spacePlayerShots[SPACE_PLAYER_SHOTS];
SpaceShot spaceEnemyShots[SPACE_ENEMY_SHOTS];
uint8_t spaceShields[SPACE_SHIELD_COUNT][SPACE_SHIELD_ROWS];
SpaceBonus spaceBonus;

int16_t spacePlayerX = 80;
int16_t spaceFormationX = 24;
int16_t spaceFormationY = 24;
int8_t spaceFormationDirection = 1;
int8_t spaceNextBonusDirection = 1;
uint8_t spaceInvadersRemaining = 40;
uint8_t spaceLives = 3;
uint16_t spaceWave = 1;
unsigned long spaceScore = 0;
bool spaceFireArmed = false;
bool spacePaused = false;
bool spaceAnimationFrame = false;
uint8_t spaceTransition = 0;
unsigned long spaceLastFrame = 0;
unsigned long spaceNextFormationAt = 0;
unsigned long spaceNextPlayerShotAt = 0;
unsigned long spaceNextEnemyShotAt = 0;
unsigned long spaceNextBonusAt = 0;
unsigned long spaceProtectedUntil = 0;
unsigned long spaceTransitionUntil = 0;
unsigned long spacePauseStartedAt = 0;

void spaceClearShots() {
  for (uint8_t i = 0; i < SPACE_PLAYER_SHOTS; ++i) {
    spacePlayerShots[i] = {0, 0, false};
  }
  for (uint8_t i = 0; i < SPACE_ENEMY_SHOTS; ++i) {
    spaceEnemyShots[i] = {0, 0, false};
  }
}

void spaceScheduleEnemyShot(unsigned long now) {
  const int waveSpeedup = (spaceWave - 1) * 60;
  const int minimumDelay = max(350, 800 - waveSpeedup);
  const int maximumDelay = max(650, 1400 - waveSpeedup);
  spaceNextEnemyShotAt = now + random(minimumDelay, maximumDelay + 1);
}

void spaceScheduleBonus(unsigned long now) {
  spaceNextBonusAt = now + random(18000, 28001);
}

void spaceResetFormation(unsigned long now) {
  for (uint8_t row = 0; row < SPACE_INVADER_ROWS; ++row) {
    for (uint8_t col = 0; col < SPACE_INVADER_COLS; ++col) {
      spaceInvaders[row][col] = true;
    }
  }
  spaceFormationX = 24;
  spaceFormationY = 24;
  spaceFormationDirection = 1;
  spaceInvadersRemaining = SPACE_INVADER_ROWS * SPACE_INVADER_COLS;
  spaceAnimationFrame = false;
  spaceNextFormationAt = now + 500;
  spaceClearShots();
  spaceScheduleEnemyShot(now);
}

void spaceResetShields() {
  const uint8_t shape[SPACE_SHIELD_ROWS] = {
      0b00111100, 0b01111110, 0b11111111, 0b11100111, 0b11000011,
  };
  for (uint8_t shield = 0; shield < SPACE_SHIELD_COUNT; ++shield) {
    for (uint8_t row = 0; row < SPACE_SHIELD_ROWS; ++row) {
      spaceShields[shield][row] = shape[row];
    }
  }
}

int16_t spaceInvaderX(uint8_t col) {
  return spaceFormationX + col * SPACE_INVADER_X_STEP;
}

int16_t spaceInvaderY(uint8_t row) {
  return spaceFormationY + row * SPACE_INVADER_Y_STEP;
}

bool spacePointInRect(int16_t x, int16_t y, int16_t left, int16_t top,
                      int16_t width, int16_t height) {
  return x >= left && x < left + width && y >= top && y < top + height;
}

bool spaceHitShield(int16_t x, int16_t y) {
  constexpr int16_t shieldTop = 88;
  const int16_t shieldLeft[SPACE_SHIELD_COUNT] = {24, 72, 120};
  if (y < shieldTop || y >= shieldTop + SPACE_SHIELD_ROWS * 3) {
    return false;
  }
  const uint8_t row = (y - shieldTop) / 3;
  for (uint8_t shield = 0; shield < SPACE_SHIELD_COUNT; ++shield) {
    if (x < shieldLeft[shield] ||
        x >= shieldLeft[shield] + SPACE_SHIELD_COLS * 2) {
      continue;
    }
    const uint8_t col = (x - shieldLeft[shield]) / 2;
    const uint8_t bit = 0x80 >> col;
    if (spaceShields[shield][row] & bit) {
      spaceShields[shield][row] &= ~bit;
      tone(BUZZER_PIN, 340, 25);
      return true;
    }
  }
  return false;
}

void spaceUpdatePlayer(const InputFrame &input) {
  if (input.y < JOYSTICK_LOW) {
    spacePlayerX -= 2;
  } else if (input.y > JOYSTICK_HIGH) {
    spacePlayerX += 2;
  }
  spacePlayerX = constrain(spacePlayerX, 8, SCREEN_W - 8);
  if (!input.rightHeld) {
    spaceFireArmed = true;
  }
}

void spaceFirePlayerShot(unsigned long now) {
  if (!spaceFireArmed || static_cast<long>(now - spaceNextPlayerShotAt) < 0) {
    return;
  }
  for (uint8_t i = 0; i < SPACE_PLAYER_SHOTS; ++i) {
    if (!spacePlayerShots[i].active) {
      spacePlayerShots[i] = {spacePlayerX, 109, true};
      spaceNextPlayerShotAt = now + 250;
      tone(BUZZER_PIN, 1050, 24);
      return;
    }
  }
}

void spaceFireEnemyShot(unsigned long now) {
  uint8_t occupiedColumns[SPACE_INVADER_COLS];
  uint8_t occupiedCount = 0;
  for (uint8_t col = 0; col < SPACE_INVADER_COLS; ++col) {
    for (int8_t row = SPACE_INVADER_ROWS - 1; row >= 0; --row) {
      if (spaceInvaders[row][col]) {
        occupiedColumns[occupiedCount++] = col;
        break;
      }
    }
  }
  if (occupiedCount > 0) {
    const uint8_t col = occupiedColumns[random(occupiedCount)];
    int8_t firingRow = -1;
    for (int8_t row = SPACE_INVADER_ROWS - 1; row >= 0; --row) {
      if (spaceInvaders[row][col]) {
        firingRow = row;
        break;
      }
    }
    for (uint8_t i = 0; i < SPACE_ENEMY_SHOTS; ++i) {
      if (!spaceEnemyShots[i].active && firingRow >= 0) {
        spaceEnemyShots[i] = {
            static_cast<int16_t>(spaceInvaderX(col) + SPACE_INVADER_W / 2),
            static_cast<int16_t>(spaceInvaderY(firingRow) + SPACE_INVADER_H),
            true};
        break;
      }
    }
  }
  spaceScheduleEnemyShot(now);
}

unsigned long spaceFormationInterval() {
  const unsigned long waveReduction =
      min(240UL, (static_cast<unsigned long>(spaceWave) - 1UL) * 35UL);
  const unsigned long waveBase = 500UL - waveReduction;
  return max(120UL, waveBase * spaceInvadersRemaining / 40UL);
}

void spaceUpdateFormation(unsigned long now) {
  if (static_cast<long>(now - spaceNextFormationAt) < 0 ||
      spaceInvadersRemaining == 0) {
    return;
  }
  int16_t left = SCREEN_W;
  int16_t right = 0;
  int16_t bottom = 0;
  for (uint8_t row = 0; row < SPACE_INVADER_ROWS; ++row) {
    for (uint8_t col = 0; col < SPACE_INVADER_COLS; ++col) {
      if (!spaceInvaders[row][col])
        continue;
      left = min(left, spaceInvaderX(col));
      right = max(right,
                  static_cast<int16_t>(spaceInvaderX(col) + SPACE_INVADER_W));
      bottom = max(bottom,
                   static_cast<int16_t>(spaceInvaderY(row) + SPACE_INVADER_H));
    }
  }
  const int16_t step = spaceFormationDirection * 3;
  if (left + step < 4 || right + step > 155) {
    spaceFormationDirection = -spaceFormationDirection;
    spaceFormationY += 4;
    bottom += 4;
  } else {
    spaceFormationX += step;
  }
  spaceAnimationFrame = !spaceAnimationFrame;
  spaceNextFormationAt = now + spaceFormationInterval();
  if (bottom >= 108) {
    endSpaceGame();
  }
}

void spaceUpdateShots() {
  for (uint8_t i = 0; i < SPACE_PLAYER_SHOTS; ++i) {
    if (!spacePlayerShots[i].active)
      continue;
    spacePlayerShots[i].y -= 4;
    if (spacePlayerShots[i].y < 10) {
      spacePlayerShots[i].active = false;
    }
  }
  for (uint8_t i = 0; i < SPACE_ENEMY_SHOTS; ++i) {
    if (!spaceEnemyShots[i].active)
      continue;
    spaceEnemyShots[i].y += 2;
    if (spaceEnemyShots[i].y > 126) {
      spaceEnemyShots[i].active = false;
    }
  }
}

void spaceStartBonus(unsigned long now) {
  spaceBonus.active = true;
  spaceBonus.direction = spaceNextBonusDirection;
  spaceNextBonusDirection = -spaceNextBonusDirection;
  spaceBonus.x = spaceBonus.direction > 0 ? -9 : SCREEN_W + 9;
  spaceNextBonusAt = 0;
  tone(BUZZER_PIN, 720, 45);
}

void spaceUpdateBonus(unsigned long now) {
  if (!spaceBonus.active) {
    if (spaceNextBonusAt != 0 &&
        static_cast<long>(now - spaceNextBonusAt) >= 0) {
      spaceStartBonus(now);
    }
    return;
  }
  spaceBonus.x += spaceBonus.direction;
  if ((spaceBonus.direction > 0 && spaceBonus.x > SCREEN_W + 9) ||
      (spaceBonus.direction < 0 && spaceBonus.x < -9)) {
    spaceBonus.active = false;
    spaceScheduleBonus(now);
  }
}

void spaceResolvePlayerShots(unsigned long now) {
  const uint16_t bonusValues[4] = {50, 100, 150, 300};
  for (uint8_t shotIndex = 0; shotIndex < SPACE_PLAYER_SHOTS; ++shotIndex) {
    SpaceShot &shot = spacePlayerShots[shotIndex];
    if (!shot.active)
      continue;
    if (spaceBonus.active &&
        spacePointInRect(shot.x, shot.y, spaceBonus.x - 8, 12, 16, 8)) {
      spaceScore += bonusValues[random(4)];
      spaceBonus.active = false;
      spaceScheduleBonus(now);
      shot.active = false;
      tone(BUZZER_PIN, 1500, 130);
      vibrateFor(60);
      continue;
    }
    bool hitInvader = false;
    for (int8_t row = SPACE_INVADER_ROWS - 1; row >= 0 && !hitInvader; --row) {
      for (uint8_t col = 0; col < SPACE_INVADER_COLS; ++col) {
        if (!spaceInvaders[row][col])
          continue;
        if (spacePointInRect(shot.x, shot.y, spaceInvaderX(col),
                             spaceInvaderY(row), SPACE_INVADER_W,
                             SPACE_INVADER_H)) {
          spaceInvaders[row][col] = false;
          --spaceInvadersRemaining;
          spaceScore += row == 0 ? 30 : row <= 2 ? 20 : 10;
          shot.active = false;
          hitInvader = true;
          tone(BUZZER_PIN, 900 + (4 - row) * 90, 45);
          break;
        }
      }
    }
    if (!hitInvader && shot.active && spaceHitShield(shot.x, shot.y)) {
      shot.active = false;
    }
  }
}

void spaceLoseLife(unsigned long now) {
  if (spaceTransition != 0 ||
      static_cast<long>(now - spaceProtectedUntil) < 0) {
    return;
  }
  spaceClearShots();
  if (spaceLives > 0)
    --spaceLives;
  spaceTransition = 1;
  spaceTransitionUntil = now + 1000;
  tone(BUZZER_PIN, 190, 220);
  vibrateFor(200);
}

void spaceResolveEnemyShots(unsigned long now) {
  for (uint8_t i = 0; i < SPACE_ENEMY_SHOTS; ++i) {
    SpaceShot &shot = spaceEnemyShots[i];
    if (!shot.active)
      continue;
    if (spaceHitShield(shot.x, shot.y)) {
      shot.active = false;
      continue;
    }
    if (static_cast<long>(now - spaceProtectedUntil) >= 0 &&
        spacePointInRect(shot.x, shot.y, spacePlayerX - 7, 110, 15, 12)) {
      shot.active = false;
      spaceLoseLife(now);
      return;
    }
  }
}

void spaceBeginNextWave(unsigned long now) {
  if (spaceTransition != 0)
    return;
  spaceScore += 500;
  spaceTransition = 2;
  spaceTransitionUntil = now + 1000;
  tone(BUZZER_PIN, 950, 100);
  delay(35);
  tone(BUZZER_PIN, 1250, 120);
}

void spaceDrawInvader(uint8_t row, uint8_t col) {
  const int16_t x = spaceInvaderX(col);
  const int16_t y = spaceInvaderY(row);
  const uint16_t color = row == 0   ? ST77XX_MAGENTA
                         : row <= 2 ? ST77XX_YELLOW
                                    : ST77XX_GREEN;
  if (row == 0) {
    tft.fillRect(x + 3, y, 6, 2, color);
    tft.fillRect(x + 1, y + 2, 10, 4, color);
    tft.drawPixel(x + 3, y + 3, ST77XX_BLACK);
    tft.drawPixel(x + 8, y + 3, ST77XX_BLACK);
  } else if (row <= 2) {
    tft.fillRect(x + 2, y + 1, 8, 6, color);
    tft.fillRect(x, y + 3, 12, 2, color);
    tft.drawPixel(x + 4, y + 3, ST77XX_BLACK);
    tft.drawPixel(x + 7, y + 3, ST77XX_BLACK);
  } else {
    tft.fillRect(x + 1, y + 2, 10, 5, color);
    tft.fillRect(x + 3, y, 6, 2, color);
    tft.drawPixel(x + 3, y + 4, ST77XX_BLACK);
    tft.drawPixel(x + 8, y + 4, ST77XX_BLACK);
  }
  const int8_t legShift = spaceAnimationFrame ? 1 : 0;
  tft.drawPixel(x + 2 + legShift, y + 7, color);
  tft.drawPixel(x + 9 - legShift, y + 7, color);
}

void spaceRenderFrame() {
  const unsigned long now = millis();
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(1, 1);
  tft.print("S:");
  tft.print(spaceScore);
  tft.setCursor(69, 1);
  tft.print("L:");
  tft.print(spaceLives);
  tft.setCursor(116, 1);
  tft.print("W:");
  tft.print(spaceWave);
  tft.drawFastHLine(0, 9, SCREEN_W, ST77XX_BLUE);

  if (spaceBonus.active) {
    tft.fillRect(spaceBonus.x - 6, 15, 12, 4, ST77XX_RED);
    tft.fillRect(spaceBonus.x - 3, 12, 6, 3, ST77XX_RED);
    tft.drawPixel(spaceBonus.x - 7, 18, ST77XX_YELLOW);
    tft.drawPixel(spaceBonus.x + 7, 18, ST77XX_YELLOW);
  }
  for (uint8_t row = 0; row < SPACE_INVADER_ROWS; ++row) {
    for (uint8_t col = 0; col < SPACE_INVADER_COLS; ++col) {
      if (spaceInvaders[row][col])
        spaceDrawInvader(row, col);
    }
  }
  const int16_t shieldLeft[SPACE_SHIELD_COUNT] = {24, 72, 120};
  for (uint8_t shield = 0; shield < SPACE_SHIELD_COUNT; ++shield) {
    for (uint8_t row = 0; row < SPACE_SHIELD_ROWS; ++row) {
      for (uint8_t col = 0; col < SPACE_SHIELD_COLS; ++col) {
        if (spaceShields[shield][row] & (0x80 >> col)) {
          tft.fillRect(shieldLeft[shield] + col * 2, 88 + row * 3, 2, 3,
                       ST77XX_GREEN);
        }
      }
    }
  }
  for (uint8_t i = 0; i < SPACE_PLAYER_SHOTS; ++i) {
    if (spacePlayerShots[i].active) {
      tft.drawFastVLine(spacePlayerShots[i].x, spacePlayerShots[i].y, 4,
                        ST77XX_CYAN);
    }
  }
  for (uint8_t i = 0; i < SPACE_ENEMY_SHOTS; ++i) {
    if (spaceEnemyShots[i].active) {
      tft.drawFastVLine(spaceEnemyShots[i].x, spaceEnemyShots[i].y, 4,
                        ST77XX_RED);
    }
  }
  const bool protectedBlink =
      static_cast<long>(spaceProtectedUntil - now) > 0 && ((now / 100) & 1);
  if (!protectedBlink && spaceTransition != 1) {
    tft.fillTriangle(spacePlayerX, 108, spacePlayerX - 7, 119, spacePlayerX + 7,
                     119, ST77XX_CYAN);
    tft.fillRect(spacePlayerX - 4, 113, 9, 7, ST77XX_CYAN);
  }
  tft.drawFastHLine(0, 126, SCREEN_W, ST77XX_BLUE);

  if (spaceTransition == 1) {
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(55, 62);
    tft.print(spaceLives == 0 ? "SHIP LOST" : "GET READY");
  } else if (spaceTransition == 2) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(49, 62);
    tft.print("WAVE CLEAR!");
  }
}

void spaceDrawPauseOverlay() {
  tft.fillRect(31, 46, 98, 35, ST77XX_BLACK);
  tft.drawRect(31, 46, 98, 35, ST77XX_CYAN);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(49, 53);
  tft.print("PAUSED");
  tft.setTextSize(1);
  tft.setCursor(42, 71);
  tft.print("LEFT: RESUME");
}

void spaceDrawGameOver() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(24, 14);
  tft.print("SPACE OVER");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(38, 48);
  tft.print("SCORE: ");
  tft.print(spaceScore);
  tft.setCursor(38, 62);
  tft.print("HIGH : ");
  tft.print(spaceHighScore);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(26, 91);
  tft.print("RIGHT: RESTART");
  tft.setCursor(38, 105);
  tft.print("LEFT: MENU");
}

void endSpaceGame() {
  if (appState != SPACE_PLAYING)
    return;
  if (spaceScore > spaceHighScore) {
    spaceHighScore = static_cast<unsigned int>(spaceScore);
    prefs.putUInt("spaceHigh", spaceHighScore);
  }
  appState = SPACE_GAME_OVER;
  spacePaused = false;
  beepGameOver();
  vibrateFor(350);
  spaceDrawGameOver();
}

void spaceResetWave(unsigned long now) {
  ++spaceWave;
  spacePlayerX = 80;
  spaceTransition = 0;
  spaceProtectedUntil = now + 1000;
  spaceResetFormation(now);
  spaceResetShields();
  spaceBonus = {0, 1, false};
  spaceScheduleBonus(now);
}

void resetSpace() {
  const unsigned long now = millis();
  appState = SPACE_PLAYING;
  spaceScore = 0;
  spaceLives = 3;
  spaceWave = 1;
  spacePlayerX = 80;
  spaceFireArmed = false;
  spacePaused = false;
  spaceTransition = 0;
  spaceNextBonusDirection = 1;
  spaceNextPlayerShotAt = now;
  spaceProtectedUntil = now + 1200;
  spaceLastFrame = now;
  spaceResetFormation(now);
  spaceResetShields();
  spaceBonus = {0, 1, false};
  spaceScheduleBonus(now);
  spaceRenderFrame();
}

void spaceShiftDeadline(unsigned long &deadline, unsigned long duration) {
  if (deadline != 0)
    deadline += duration;
}

void updateSpace(const InputFrame &input) {
  const unsigned long now = millis();
  if (input.leftPressed) {
    if (!spacePaused) {
      spacePaused = true;
      spacePauseStartedAt = now;
      spaceDrawPauseOverlay();
    } else {
      const unsigned long pausedFor = now - spacePauseStartedAt;
      spacePaused = false;
      spaceShiftDeadline(spaceNextFormationAt, pausedFor);
      spaceShiftDeadline(spaceNextPlayerShotAt, pausedFor);
      spaceShiftDeadline(spaceNextEnemyShotAt, pausedFor);
      spaceShiftDeadline(spaceNextBonusAt, pausedFor);
      spaceShiftDeadline(spaceProtectedUntil, pausedFor);
      spaceShiftDeadline(spaceTransitionUntil, pausedFor);
      spaceLastFrame = now;
      spaceRenderFrame();
    }
    return;
  }
  if (spacePaused)
    return;

  if (spaceTransition != 0) {
    if (static_cast<long>(now - spaceTransitionUntil) >= 0) {
      if (spaceTransition == 1) {
        if (spaceLives == 0) {
          endSpaceGame();
          return;
        }
        spaceTransition = 0;
        spacePlayerX = 80;
        spaceProtectedUntil = now + 1500;
        spaceScheduleEnemyShot(now);
      } else {
        spaceResetWave(now);
      }
      spaceLastFrame = now;
      spaceRenderFrame();
    }
    return;
  }

  if (now - spaceLastFrame < SPACE_FRAME_MS)
    return;
  spaceLastFrame = now;
  spaceUpdatePlayer(input);
  if (input.rightHeld)
    spaceFirePlayerShot(now);
  spaceUpdateShots();
  spaceUpdateFormation(now);
  if (appState != SPACE_PLAYING)
    return;
  if (static_cast<long>(now - spaceNextEnemyShotAt) >= 0) {
    spaceFireEnemyShot(now);
  }
  spaceUpdateBonus(now);
  spaceResolvePlayerShots(now);
  spaceResolveEnemyShots(now);
  if (spaceTransition == 0 && spaceInvadersRemaining == 0) {
    spaceBeginNextWave(now);
  }
  spaceRenderFrame();
}

void updateSpaceGameOver(const InputFrame &input) {
  if (input.rightPressed)
    resetSpace();
  else if (input.leftPressed)
    enterMenu();
}

//==================================================
// TELEMETRY LOGIC
//==================================================
void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 60);
  tft.print("Connecting to WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    delay(250);
  }
}

volatile int telemetryScore = 0;
volatile int telemetryLevel = 1;
char telemetryGame[16] = "Menu";
volatile bool telemetryPending = false;

void telemetryTask(void * pvParameters) {
  for(;;) {
    if (telemetryPending) {
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.setTimeout(2500);
        http.begin(SERVER_URL);
        http.addHeader("Content-Type", "application/json");

        StaticJsonDocument<384> payload;
        payload["name"] = playerName;
        payload["branch"] = branchName;
        payload["game"] = String(telemetryGame);
        payload["score"] = telemetryScore;
        payload["level"] = telemetryLevel;
        payload["battery"] = 100;
        payload["wifi"] = WiFi.RSSI();
        payload["fps"] = 30;
        payload["heap"] = ESP.getFreeHeap();

        String body;
        serializeJson(payload, body);
        http.POST(body);
        http.end();
      }
      telemetryPending = false;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); // Yield to other tasks
  }
}

//==================================================
// ARDUINO ENTRY POINTS
//==================================================
void setup() {
  Serial.begin(115200);

  pinMode(LEFT_SW, INPUT_PULLUP);
  pinMode(RIGHT_SW, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VIBRATION_PIN, OUTPUT);
  digitalWrite(VIBRATION_PIN, LOW);

  analogReadResolution(12);
  randomSeed(analogRead(LEFT_X));

  prefs.begin("snake", false);
  snakeHighScore = prefs.getUInt("high", 0);
  tetrisHighScore = prefs.getUInt("tetrisHigh", 0);
  doomHighScore = prefs.getUInt("doomHigh", 0);
  racerHighScore = prefs.getUInt("raceHigh", 0);
  pacHighScore = prefs.getUInt("pacHigh", 0);
  spaceHighScore = prefs.getUInt("spaceHigh", 0);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  connectWifi();
  
  xTaskCreatePinnedToCore(
    telemetryTask,   /* Task function. */
    "TelemetryTask", /* name of task. */
    8192,            /* Stack size of task */
    NULL,            /* parameter of the task */
    1,               /* priority of the task */
    NULL,            /* Task handle to keep track of created task */
    0);              /* pin task to core 0 */

  enterMenu();
}

void loop() {
  serviceHaptics();

  const InputFrame input = readInput();

  switch (appState) {
  case GAME_MENU:
    updateMenu(input);
    break;
  case SNAKE_PLAYING:
    updateSnake(input);
    break;
  case SNAKE_GAME_OVER:
    updateSnakeGameOver(input);
    break;
  case TETRIS_PLAYING:
    updateTetris(input);
    break;
  case TETRIS_GAME_OVER:
    updateTetrisGameOver(input);
    break;
  case DOOM_PLAYING:
    updateDoom(input);
    break;
  case DOOM_GAME_OVER:
    updateDoomGameOver(input);
    break;
  case RACER_PLAYING:
    updateRacer(input);
    break;
  case RACER_GAME_OVER:
    updateRacerGameOver(input);
    break;
  case PAC_PLAYING:
    updatePac(input);
    break;
  case PAC_GAME_OVER:
    updatePacGameOver(input);
    break;
  case SPACE_PLAYING:
    updateSpace(input);
    break;
  case SPACE_GAME_OVER:
    updateSpaceGameOver(input);
    break;
  }

  // Periodic Telemetry Updates (Triggering Task)
  if (millis() - lastPostMs >= POST_INTERVAL_MS && !telemetryPending) {
    lastPostMs = millis();
    
    if (appState == SNAKE_PLAYING) {
      strncpy(telemetryGame, "Snake", sizeof(telemetryGame));
      telemetryScore = snakeScore; telemetryLevel = 1; telemetryPending = true;
    } else if (appState == TETRIS_PLAYING) {
      strncpy(telemetryGame, "Tetris", sizeof(telemetryGame));
      telemetryScore = tetrisScore; telemetryLevel = tetrisLevel; telemetryPending = true;
    } else if (appState == DOOM_PLAYING) {
      strncpy(telemetryGame, "Doom", sizeof(telemetryGame));
      telemetryScore = doomScore; telemetryLevel = 1; telemetryPending = true;
    } else if (appState == RACER_PLAYING) {
      strncpy(telemetryGame, "Racer", sizeof(telemetryGame));
      telemetryScore = racerHighScore; telemetryLevel = 1; telemetryPending = true;
    } else if (appState == PAC_PLAYING) {
      strncpy(telemetryGame, "Pac-Maze", sizeof(telemetryGame));
      telemetryScore = pacHighScore; telemetryLevel = 1; telemetryPending = true;
    } else if (appState == SPACE_PLAYING) {
      strncpy(telemetryGame, "Space Raid", sizeof(telemetryGame));
      telemetryScore = spaceHighScore; telemetryLevel = 1; telemetryPending = true;
    }
  }

  delay(5);
}
