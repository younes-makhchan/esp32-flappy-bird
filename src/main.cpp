#include <Arduino.h>
#include <MD_MAX72xx.h>
#include <EasyButton.h>
#include "tetris.h"   // <-- Your tetris engine header

// ================== CONFIG ==================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   4

#define CLK_PIN       18
#define DATA_PIN      23
#define CS_PIN        5

#define BUTTON_A      32
#define BUTTON_B      33

#define BTN_LONG_DURATION 300
#define TEXT_SCROLL_DELAY 50
// ============================================

// Global objects
MD_MAX72XX mx(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
EasyButton btn_a(BUTTON_A);
EasyButton btn_b(BUTTON_B);

game tetris;

// ============ FUNCTIONS =====================

void check_btns();

void scrollText(const char *p)
{
  uint8_t charWidth;
  uint8_t cBuf[8];

  mx.clear();

  while (*p != '\0')
  {
    charWidth = mx.getChar(*p++, sizeof(cBuf), cBuf);

    for (uint8_t i = 0; i <= charWidth; i++)
    {
      mx.transform(MD_MAX72XX::TSL);
      if (i < charWidth)
        mx.setColumn(0, cBuf[i]);

      delay(TEXT_SCROLL_DELAY);
    }
  }
}

void draw(game *g)
{
  int mx_cols = mx.getColumnCount();
  int col_count = 0;

  // Draw next piece
  u16 *next = g->next + 1;

  for (int j = 0; j < T; ++j)
  {
    check_btns();
    mx.setColumn(mx_cols - 2 - col_count++ - 1, next[j] << 3);
  }

  mx.setColumn(mx_cols - 8, 0xff);

  // Draw board
  col_count = 0;
  for (int j = T; j < g->h + T; ++j)
  {
    check_btns();
    mx.setColumn(mx_cols - 8 - col_count++ - 1, g->board_dr[j]);
  }
}

void draw_game_over(game *g)
{
  mx.clear();
  scrollText("GAME OVER");

  char buf[16];
  itoa(g->score, buf, 10);
  scrollText(buf);

  delay(3000);
  mx.clear();
}

// ============ BUTTON CALLBACKS ==============

// Button A short press: Move Left
void on_btn_a() { tetris.msg = LEFT; }

// Button A long press: Rotate
void on_btn_a_2() { tetris.msg = ROTATE2; }

// Button B short press: Move Right
void on_btn_b() { tetris.msg = RIGHT; }

// Button B long press: Drop object
void animated_drop(game *g)
{
  while (true)
  {
    check_btns();
    int prev_y = g->curr_y;
    move_shape(g, DOWN);          // move one step down via the engine
    draw(g);
    delay(40);                    // pause so the eye can follow it
    if (g->curr_y == prev_y) break; // piece didn't move = it locked, stop
  }
}

// Button B long press: Animated drop
void on_btn_b_2() { animated_drop(&tetris); }

void check_btns()
{
  btn_a.read();
  btn_b.read();
}

// =============== SETUP =======================

void setup()
{
  mx.begin();
  mx.clear();
  mx.control(MD_MAX72XX::INTENSITY, 2);

  btn_a.begin();
  btn_a.onPressed(on_btn_a);
  btn_a.onPressedFor(BTN_LONG_DURATION, on_btn_a_2);

  btn_b.begin();
  btn_b.onPressed(on_btn_b);
  btn_b.onPressedFor(BTN_LONG_DURATION, on_btn_b_2);

  tetris.w = 8;
  tetris.h = 24;

  game_init(&tetris);
}

// =============== LOOP ========================

void loop()
{
  check_btns();
  game_loop(&tetris);
}