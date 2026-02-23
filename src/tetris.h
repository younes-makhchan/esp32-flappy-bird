#include <stdint.h>
#include <stdbool.h>

// dimensions
#define W_MAX 16 // max board width
#define H_MAX 40 // max board height
#define W     10 // normal board width
#define H     20 // normal board height
#define T     4  // shape dimension

// messages
#define NONE   0
#define QUIT   1
#define REDRAW 2
#define PAUSE  3

// move directions and messages
#define BOTTOM  4
#define BOTTOM2 10
#define DOWN    5
#define RIGHT   6
#define LEFT    7
#define ROTATE1 8
#define ROTATE2 9

// move status
#define FAIL 0
#define OK   1
#define ADD  2

// miscellaneous
#define LINE(g) ((u16) ~(((u16) ~0) << g->w))
#define CELL(matrix, i, j) ((matrix[j] >> (i)) & 1)

#define S_HEIGHT(shape)   ((*(shape) >>  0) & 0xf)
#define S_WIDTH(shape)    ((*(shape) >>  4) & 0xf)
#define S_ROTATION(shape) ((*(shape) >>  8) & 0xf)
#define S_TYPE(shape)     ((*(shape) >> 12) & 0xf)

#define u16 uint16_t
#define ul  unsigned long

// tetriminos shapes
// first element is metadata of type,rotation,width,height; 4 bits each
// type can be used for scoring and coloring
// rotation can be used for scoring
// width and height are used to save calculations time
extern u16 shapes[][T + 1];

// based on type and rotation
// from emacs tetris-shape-scores
extern int shape_scores[][4];

typedef struct {
  void *helper;              // drawing helper
  int   w;                   // width
  int   h;                   // height
  u16   board[H_MAX + T];    // main board matrix
  u16   board_dr[H_MAX + T]; // drawing matrix
  u16   current[T + 1];      // current shape
  u16   next[T + 1];         // next shape
  int   shapes;              // shapes count
  int   rows;                // completed rows count
  int   score;               // score count
  int   curr_x;              // current shape x position
  int   curr_y;              // current shape y position
  int   board_sum;           // used for collision detection
  ul    time_now;            // timer
  int   period;              // current level delay period (ms)
  int   status;              // current move status
  bool  running;             // game is running
  bool  paused;              // game is paused
  int   msg;                 // messages to be run in the loop
  bool  bottom;              // jumping to the bottom
} game;

// game interface
void game_init(game *g);
void game_loop(game *g);

// utils
int  rand_int();
void rand_seed();
ul   millis();

// drawing utils
void draw(game *g);
void draw_game_over(game *g);