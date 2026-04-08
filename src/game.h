#ifndef GAME_H
#define GAME_H

#include <stdlib.h>

/* Board dimensions */
#define BOARD_WIDTH     10
#define BOARD_HEIGHT    20

/* Piece system */
#define PIECE_COUNT     7
#define ROTATION_COUNT  4
#define PIECE_SIZE      4

/* ncurses color pair indices */
#define COLOR_PAIR_I      1
#define COLOR_PAIR_O      2
#define COLOR_PAIR_T      3
#define COLOR_PAIR_S      4
#define COLOR_PAIR_Z      5
#define COLOR_PAIR_J      6
#define COLOR_PAIR_L      7
#define COLOR_PAIR_BORDER 8
#define COLOR_PAIR_UI     9

/* Piece type indices */
#define PIECE_I 0
#define PIECE_O 1
#define PIECE_T 2
#define PIECE_S 3
#define PIECE_Z 4
#define PIECE_J 5
#define PIECE_L 6

/* Lines needed to advance one level */
#define LINES_PER_LEVEL 10

typedef struct {
    /* Board: 0 = empty, 1-7 = locked piece color pair index */
    int board[BOARD_HEIGHT][BOARD_WIDTH];

    /* Current falling piece */
    int current_piece;
    int current_rotation;
    int current_x;
    int current_y;

    /* Next piece */
    int next_piece;

    /* 7-bag randomizer */
    int bag[PIECE_COUNT];
    int bag_index;

    /* Game statistics */
    int score;
    int level;
    int lines_cleared;

    /* Flags */
    int game_over;
    int paused;

    /* Gravity tick counter */
    int gravity_counter;
} GameState;

/* Piece shapes: [piece][rotation][row][col] — 1 = filled */
extern const int PIECE_SHAPES[PIECE_COUNT][ROTATION_COUNT][PIECE_SIZE][PIECE_SIZE];

/* Color pair index for each piece type */
extern const int PIECE_COLORS[PIECE_COUNT];

/* Public API */
void game_init(GameState *g);
int  game_check_collision(const GameState *g, int piece, int rotation, int x, int y);
int  game_move(GameState *g, int dx, int dy);
void game_rotate(GameState *g, int dir);
void game_hard_drop(GameState *g);
void game_tick(GameState *g);
int  game_get_ghost_y(const GameState *g);

#endif /* GAME_H */
