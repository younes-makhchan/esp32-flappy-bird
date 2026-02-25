#include <Arduino.h>
#include "tetris.h"

void rand_seed() {
    randomSeed(millis());
}

int rand_int() {
    return random(0, 100);
}
u16 shapes[][T + 1] = {
  {0x0022,
   0b0011,
   0b0011,
   0b0000,
   0b0000},

  {0x1032,
   0b0111,
   0b0100,
   0b0000,
   0b0000},

  {0x2032,
   0b0111,
   0b0001,
   0b0000,
   0b0000},

  {0x3032,
   0b0011,
   0b0110,
   0b0000,
   0b0000},

  {0x4032,
   0b0110,
   0b0011,
   0b0000,
   0b0000},

  {0x5032,
   0b0010,
   0b0111,
   0b0000,
   0b0000},

  {0x6041,
   0b1111,
   0b0000,
   0b0000,
   0b0000}
};

int shape_scores[][4] = {
  {6, 6, 6, 6},
  {6, 7, 6, 7},
  {6, 7, 6, 7},
  {6, 7, 6, 7},
  {6, 7, 6, 7},
  {5, 5, 6, 5},
  {5, 8, 5, 8},
};

void zero(u16 *matrix, int h) {
  for (int j = 0; j < h; ++j) {
    matrix[j] = 0;
  }
}

// put shape s to board b at x,y
void put_shape(game *g, u16 *s, u16 *b, int x, int y) {
  ++s; // skip metadata
  int row, index;
  for (int j = 0; j < T; ++j) {
    row = s[j] << x;
    index = y + j;
    if (index >= 0 && index < (g->h + T)) {
      b[index] |= row;
      b[index] &= LINE(g);
    }
  }
}

void rotate(u16 *s, int times, int dir) {
  static u16 tmp[T];
  ++s; // skip metadata

  if (dir == RIGHT) {
    times += 2;
  }

  times %= 4;

  // rotate multiple times
  for (int _ = 0; _ < times; ++_) {
    // clean the tmp shape
    for (int j = 0; j < T; ++j) {
      tmp[j] = 0;
    }

    // rotate to tmp
    for (int j = 0; j < T; ++j) {
      for (int i = 0; i < T; ++i) {
        tmp[i] |= CELL(s, T - i - 1, j) << j;
      }
    }

    // move up
    for (int _ = 0; _ < T; ++_) {
      if (tmp[0] == 0) {
        for (int j = 0; j < T - 1; ++j) {
          tmp[j] = tmp[j + 1];
        }
        tmp[T - 1] = 0;
      } else {
        break;
      }
    }

    // move left
    bool all_zeros = true;
    for (int _ = 0; _ < T; ++_) {
      for (int j = 0; j < T; ++j) {
        if (CELL(tmp, 0, j) != 0) {
          all_zeros = false;
          break;
        }
      }
      if (all_zeros) {
        for (int j = 0; j < T; ++j) {
          tmp[j] = tmp[j] << 1;
        }
      } else {
        break;
      }
    }

    // copy back
    for (int j = 0; j < T; ++j) {
      s[j] = tmp[j];
    }
  }

  // update metadata
  --s;
  if (times % 2 != 0) {
    int w = S_WIDTH(s);
    int h = S_HEIGHT(s);
    *s &= 0xff00;
    *s |= (h << 4) | w;
  }
  times = (S_ROTATION(s) + times) % 4;
  *s &= 0xf0ff;
  *s |= times << 8;
}

void copy(u16 *src, u16 *dst, int h) {
  for (int j = 0; j < h; ++j) {
    dst[j] = src[j];
  }
}

// used to check for collisions
int sum(u16 *matrix, int w, int h) {
  int res = 0;
  for (int j = 0; j < h; ++j) {
    for (int i = 0; i < w; ++i) {
      res += CELL(matrix, i, j);
    }
  }
  return res;
}

void random_next(game *g) {
  copy(shapes[rand_int() % 7], g->next, T + 1);
  rotate(g->next, rand_int() % 4, LEFT);
}

void next(game *g) {
  copy(g->next, g->current, T + 1);
  random_next(g);
  g->curr_x = (g->w / 2) - (S_WIDTH(g->current) / 2);
  g->curr_y = T - S_HEIGHT(g->current);
}

void game_init(game *g) {
  g->board_sum = 0;
  g->shapes    = 0;
  g->rows      = 0;
  g->score     = 0;
  g->time_now  = millis();
  g->running   = true;
  g->paused    = false;
  g->bottom    = false;

  zero(g->board, g->h + T);
  rand_seed();
  random_next(g);
  next(g);
}

int move_shape(game *g, int dir) {
  int new_x = g->curr_x;
  int new_y = g->curr_y;
  int w;

  switch (dir) {
  case DOWN:
    new_y += 1;
    break;

  case RIGHT:
    new_x += 1;
    break;

  case LEFT:
    new_x -= 1;
    break;

  case BOTTOM:
    g->bottom = true;
    return OK;

  case BOTTOM2:
    while (move_shape(g, DOWN) == OK);
    return OK;

  case ROTATE1:
  case ROTATE2:
    rotate(g->current, 1, dir == ROTATE1 ? RIGHT : LEFT);
    w = S_WIDTH(g->current);
    if ((new_x + w) > g->w) {
      new_x = g->w - w;
    }
    break;

  default:
    return FAIL;
  }

  // left or right wall
  if (new_x < 0 || (new_x + S_WIDTH(g->current)) > g->w) {
    return FAIL;
  }

  // bottom
  if ((new_y + S_HEIGHT(g->current)) > (g->h + T)) {
    return ADD;
  }

  copy(g->board, g->board_dr, g->h + T);

  int sum_expected = sum(g->board_dr, g->w, g->h + T) + sum(g->current + 1, T, T);
  put_shape(g, g->current, g->board_dr, new_x, new_y);
  int sum_res = sum(g->board_dr, g->w, g->h + T);

  // collision
  if (sum_expected != sum_res) {
    if (dir == ROTATE1) { // unrotate
      rotate(g->current, 1, LEFT);
    } else if (dir == ROTATE2) {
      rotate(g->current, 1, RIGHT);
    }
    copy(g->board, g->board_dr, g->h + T);
    put_shape(g, g->current, g->board_dr, g->curr_x, g->curr_y);
    return ADD;
  }

  g->curr_x = new_x;
  g->curr_y = new_y;

  return OK;
}
void game_loop(game *g) {
  if (g->msg != NONE) {
    switch (g->msg) {
    case QUIT:
      g->running = false;
      break;

    case REDRAW:
      draw(g);
      break;

    case PAUSE:
      if (g->paused) {
        g->time_now = millis();
        g->paused = false;
      } else {
        g->paused = true;
      }
      break;

    case BOTTOM:
    case BOTTOM2:
    case DOWN:
    case LEFT:
    case RIGHT:
    case ROTATE1:
    case ROTATE2:
      move_shape(g, g->msg);
      draw(g);
      break;
    }

    g->msg = NONE;

  } else if (g->bottom) {
    if (move_shape(g, DOWN) == OK) {
      draw(g);
    } else {
      g->bottom = false;
    }
  } else if (!g->paused) {
    // calculate game speed
    g->period = (int) ((20.0 / (50.0 + g->rows)) * 1000.0);
    
    if (millis() >= g->time_now + g->period) {
      g->time_now += g->period;

      // game over, reset
      if (g->board[T - 1] != 0) {
        draw_game_over(g);
        game_init(g);
        return;
      }

      // normal move down
      g->status = move_shape(g, DOWN);
      if (g->status == FAIL) {
        return;
      } else if (g->status == ADD) {
        ++g->shapes;
        g->score += shape_scores[S_TYPE(g->current)][S_ROTATION(g->current)];
        put_shape(g, g->current, g->board, g->curr_x, g->curr_y);
        next(g); // spawn next shape
      }

      // --- NEW FLASHING LOGIC ---
      
      // 1. Check if any lines are fully completed
      bool lines_cleared = false;
      for (int j = g->h + T - 1; j >= T; --j) {
        if (g->board[j] == LINE(g)) {
          lines_cleared = true;
          break;
        }
      }

      // 2. Animate the flash if lines are complete
      if (lines_cleared) {
        // Flash 3 times
        for (int flash = 0; flash < 3; ++flash) {
          // Turn off completed lines
          for (int j = T; j < g->h + T; ++j) {
            if (g->board[j] == LINE(g)) g->board_dr[j] = 0; 
            else g->board_dr[j] = g->board[j];
          }
          draw(g);
          delay(100); // Wait 100ms

          // Turn on completed lines
          for (int j = T; j < g->h + T; ++j) {
            if (g->board[j] == LINE(g)) g->board_dr[j] = LINE(g); 
            else g->board_dr[j] = g->board[j];
          }
          draw(g);
          delay(100); // Wait 100ms
        }

        // 3. Actually remove the lines and collapse the board
        for (int j = g->h + T - 1; j >= T; --j) {
          if (g->board[j] == LINE(g)) {
            g->board[j] = 0;
            for (int k = j; k >= T; --k) {
              g->board[k] = g->board[k - 1];
            }
            ++j;
            ++g->rows;
          }
        }
        
        // 4. Update the drawing board so the new piece doesn't flicker
        copy(g->board, g->board_dr, g->h + T);
        put_shape(g, g->current, g->board_dr, g->curr_x, g->curr_y);
      }
      // --- END NEW FLASHING LOGIC ---

      draw(g);
    }
  }
}