#include "game.h"
#include <string.h>

/* ─── Piece shape data ──────────────────────────────────────────────────── */

/*
 * Each piece has 4 rotation states stored as a 4×4 binary grid.
 * Rows go top→bottom, columns left→right.
 */
const int PIECE_SHAPES[PIECE_COUNT][ROTATION_COUNT][PIECE_SIZE][PIECE_SIZE] = {
    /* PIECE_I (0) — cyan */
    {
        {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
        {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}
    },
    /* PIECE_O (1) — yellow */
    {
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}
    },
    /* PIECE_T (2) — magenta */
    {
        {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}
    },
    /* PIECE_S (3) — green */
    {
        {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
        {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
        {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}
    },
    /* PIECE_Z (4) — red */
    {
        {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
        {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}}
    },
    /* PIECE_J (5) — blue */
    {
        {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}
    },
    /* PIECE_L (6) — white */
    {
        {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
        {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}
    }
};

const int PIECE_COLORS[PIECE_COUNT] = {
    COLOR_PAIR_I,
    COLOR_PAIR_O,
    COLOR_PAIR_T,
    COLOR_PAIR_S,
    COLOR_PAIR_Z,
    COLOR_PAIR_J,
    COLOR_PAIR_L
};

/* ─── Internal helpers ──────────────────────────────────────────────────── */

/* Returns the gravity interval (ticks between drops) for the given level. */
static int gravity_period(int level)
{
    /* Inspired by NES Tetris frame counts, scaled to 50 ms ticks */
    static const int periods[20] = {
        48, 43, 38, 33, 28, 23, 18, 13, 8, 6,
         5,  5,  5,  4,  4,  4,  3,  3,  3, 2
    };
    if (level < 1)  level = 1;
    if (level > 20) level = 20;
    return periods[level - 1];
}

/* Fisher–Yates shuffle of the 7-bag.
 * Uses the rejection-sampling approach to avoid modulo bias. */
static void shuffle_bag(GameState *g)
{
    int i, j, tmp;
    for (i = 0; i < PIECE_COUNT; i++)
        g->bag[i] = i;
    for (i = PIECE_COUNT - 1; i > 0; i--) {
        int range = i + 1;
        int limit  = RAND_MAX - (RAND_MAX % range);
        int r;
        do { r = rand(); } while (r >= limit);
        j = r % range;
        tmp = g->bag[i];
        g->bag[i] = g->bag[j];
        g->bag[j] = tmp;
    }
    g->bag_index = 0;
}

/* Draw the next piece from the bag; reshuffle when exhausted. */
static int draw_from_bag(GameState *g)
{
    if (g->bag_index >= PIECE_COUNT)
        shuffle_bag(g);
    return g->bag[g->bag_index++];
}

/* Spawn the current piece at the top of the board. Sets game_over if blocked. */
static void spawn_piece(GameState *g)
{
    g->current_piece    = g->next_piece;
    g->next_piece       = draw_from_bag(g);
    g->current_rotation = 0;
    g->current_x        = BOARD_WIDTH  / 2 - PIECE_SIZE / 2; /* = 3 */
    g->current_y        = 0;
    g->gravity_counter  = 0;

    if (game_check_collision(g, g->current_piece, g->current_rotation,
                             g->current_x, g->current_y))
        g->game_over = 1;
}

/* Copy the current piece onto the board, clear lines, spawn next piece. */
static void lock_piece(GameState *g)
{
    int row, col;
    int color = PIECE_COLORS[g->current_piece];

    for (row = 0; row < PIECE_SIZE; row++) {
        for (col = 0; col < PIECE_SIZE; col++) {
            if (!PIECE_SHAPES[g->current_piece][g->current_rotation][row][col])
                continue;
            int bx = g->current_x + col;
            int by = g->current_y + row;
            if (by >= 0 && by < BOARD_HEIGHT && bx >= 0 && bx < BOARD_WIDTH)
                g->board[by][bx] = color;
        }
    }

    /* Clear complete lines */
    {
        static const int line_scores[5] = {0, 100, 300, 500, 800};
        int lines = 0;
        row = BOARD_HEIGHT - 1;
        while (row >= 0) {
            int full = 1;
            for (col = 0; col < BOARD_WIDTH; col++) {
                if (!g->board[row][col]) { full = 0; break; }
            }
            if (full) {
                int r;
                lines++;
                for (r = row; r > 0; r--)
                    memcpy(g->board[r], g->board[r - 1], sizeof(g->board[r]));
                memset(g->board[0], 0, sizeof(g->board[0]));
                /* Re-check the same row index after shifting */
            } else {
                row--;
            }
        }
        if (lines > 0) {
            int s = (lines <= 4) ? line_scores[lines] : 800;
            g->score        += s * g->level;
            g->lines_cleared += lines;
            g->level         = g->lines_cleared / LINES_PER_LEVEL + 1;
            if (g->level > 20) g->level = 20;
        }
    }

    spawn_piece(g);
}

/* ─── Public API ────────────────────────────────────────────────────────── */

void game_init(GameState *g)
{
    memset(g, 0, sizeof(GameState));
    g->level = 1;
    shuffle_bag(g);
    g->next_piece = draw_from_bag(g);
    spawn_piece(g);
}

int game_check_collision(const GameState *g, int piece, int rotation, int x, int y)
{
    int row, col;
    for (row = 0; row < PIECE_SIZE; row++) {
        for (col = 0; col < PIECE_SIZE; col++) {
            if (!PIECE_SHAPES[piece][rotation][row][col])
                continue;
            int bx = x + col;
            int by = y + row;
            if (bx < 0 || bx >= BOARD_WIDTH)  return 1;
            if (by >= BOARD_HEIGHT)            return 1;
            if (by < 0)                        continue; /* above board is ok */
            if (g->board[by][bx])              return 1;
        }
    }
    return 0;
}

int game_move(GameState *g, int dx, int dy)
{
    if (game_check_collision(g, g->current_piece, g->current_rotation,
                             g->current_x + dx, g->current_y + dy))
        return 0;
    g->current_x += dx;
    g->current_y += dy;
    return 1;
}

void game_rotate(GameState *g, int dir)
{
    /* Wall-kick offsets to try in order: none, left, right, left×2, right×2, up */
    static const int kicks[6][2] = {
        {0, 0}, {-1, 0}, {1, 0}, {-2, 0}, {2, 0}, {0, -1}
    };
    int i;
    int new_rot = (g->current_rotation + dir + ROTATION_COUNT) % ROTATION_COUNT;

    for (i = 0; i < 6; i++) {
        if (!game_check_collision(g, g->current_piece, new_rot,
                                  g->current_x + kicks[i][0],
                                  g->current_y + kicks[i][1])) {
            g->current_x        += kicks[i][0];
            g->current_y        += kicks[i][1];
            g->current_rotation  = new_rot;
            return;
        }
    }
}

void game_hard_drop(GameState *g)
{
    while (game_move(g, 0, 1))
        g->score += 2 * g->level; /* bonus points for hard-dropping */
    lock_piece(g);
}

void game_tick(GameState *g)
{
    if (g->game_over || g->paused)
        return;

    g->gravity_counter++;
    if (g->gravity_counter >= gravity_period(g->level)) {
        g->gravity_counter = 0;
        if (!game_move(g, 0, 1))
            lock_piece(g);
    }
}

int game_get_ghost_y(const GameState *g)
{
    int ghost_y = g->current_y;
    while (!game_check_collision(g, g->current_piece, g->current_rotation,
                                 g->current_x, ghost_y + 1))
        ghost_y++;
    return ghost_y;
}
