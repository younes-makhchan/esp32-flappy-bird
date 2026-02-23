#include <Arduino.h>
#include <MD_MAX72xx.h>
#include "tetris.h"

// ================== CONFIG ==================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   4

#define CLK_PIN       18
#define DATA_PIN      23
#define CS_PIN        5
// ============================================

// Global objects
MD_MAX72XX mx(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

game tetris;
int dir;

// ============ FUNCTIONS =====================

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

      delay(50);
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
    mx.setColumn(mx_cols - 2 - col_count++ - 1, next[j] << 3);
  }

  mx.setColumn(mx_cols - 8, 0xff);

  // Draw board
  col_count = 0;
  for (int j = T; j < g->h + T; ++j)
  {
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

// =============== SETUP =======================

void setup()
{
  mx.begin();
  mx.clear();
  mx.control(MD_MAX72XX::INTENSITY, 1);

  dir = LEFT;

  tetris.w = 8;
  tetris.h = 24;

  game_init(&tetris);
}

// =============== LOOP ========================

void loop()
{
  // No buttons for now
  game_loop(&tetris);
}