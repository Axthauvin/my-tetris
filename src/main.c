#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "game.h"

/* ─── Display constants ─────────────────────────────────────────────────── */
#define CELL_W      2       /* each board cell is 2 characters wide */
#define TICK_MS    50       /* milliseconds per game tick            */
#define SIDEBAR_W  22       /* width of the right-hand info panel    */

/* ─── Color-pair setup ──────────────────────────────────────────────────── */
static void init_colors(void)
{
    start_color();
    use_default_colors();
    /* Piece colors: solid block rendered as two spaces on colored background */
    init_pair(COLOR_PAIR_I,      COLOR_CYAN,    COLOR_CYAN);
    init_pair(COLOR_PAIR_O,      COLOR_YELLOW,  COLOR_YELLOW);
    init_pair(COLOR_PAIR_T,      COLOR_MAGENTA, COLOR_MAGENTA);
    init_pair(COLOR_PAIR_S,      COLOR_GREEN,   COLOR_GREEN);
    init_pair(COLOR_PAIR_Z,      COLOR_RED,     COLOR_RED);
    init_pair(COLOR_PAIR_J,      COLOR_BLUE,    COLOR_BLUE);
    init_pair(COLOR_PAIR_L,      COLOR_WHITE,   COLOR_WHITE);
    init_pair(COLOR_PAIR_BORDER, COLOR_WHITE,   COLOR_BLACK);
    init_pair(COLOR_PAIR_UI,     COLOR_WHITE,   COLOR_BLACK);
}

/* ─── Low-level cell drawing ─────────────────────────────────────────────── */

/* Draw a solid 2-char block at (win_row, win_col) using the given color pair.
 * color_pair == 0 → empty cell.
 * ghost_color > 0 → dimmed ghost cell. */
static void draw_solid(WINDOW *w, int win_row, int win_col, int color_pair)
{
    wattron(w, COLOR_PAIR(color_pair));
    mvwaddstr(w, win_row, win_col, "  ");
    wattroff(w, COLOR_PAIR(color_pair));
}

static void draw_ghost(WINDOW *w, int win_row, int win_col, int color_pair)
{
    wattron(w, A_DIM | COLOR_PAIR(color_pair));
    mvwaddstr(w, win_row, win_col, "░░");
    wattroff(w, A_DIM | COLOR_PAIR(color_pair));
}

/* ─── Board window ───────────────────────────────────────────────────────── */

static void draw_board(WINDOW *bw, const GameState *g)
{
    int row, col;
    int ghost_y = game_get_ghost_y(g);

    werase(bw);

    /* Locked cells */
    for (row = 0; row < BOARD_HEIGHT; row++) {
        for (col = 0; col < BOARD_WIDTH; col++) {
            int cp = g->board[row][col];
            if (cp)
                draw_solid(bw, row + 1, col * CELL_W + 1, cp);
        }
    }

    /* Ghost piece (only when piece hasn't already reached the floor) */
    if (ghost_y != g->current_y) {
        int ghost_color = PIECE_COLORS[g->current_piece];
        for (row = 0; row < PIECE_SIZE; row++) {
            for (col = 0; col < PIECE_SIZE; col++) {
                if (!PIECE_SHAPES[g->current_piece][g->current_rotation][row][col])
                    continue;
                int bx = g->current_x + col;
                int by = ghost_y + row;
                if (by >= 0 && by < BOARD_HEIGHT && bx >= 0 && bx < BOARD_WIDTH
                        && !g->board[by][bx])
                    draw_ghost(bw, by + 1, bx * CELL_W + 1, ghost_color);
            }
        }
    }

    /* Active piece */
    for (row = 0; row < PIECE_SIZE; row++) {
        for (col = 0; col < PIECE_SIZE; col++) {
            if (!PIECE_SHAPES[g->current_piece][g->current_rotation][row][col])
                continue;
            int bx = g->current_x + col;
            int by = g->current_y + row;
            if (by >= 0 && by < BOARD_HEIGHT && bx >= 0 && bx < BOARD_WIDTH)
                draw_solid(bw, by + 1, bx * CELL_W + 1,
                           PIECE_COLORS[g->current_piece]);
        }
    }

    /* Border */
    wattron(bw, COLOR_PAIR(COLOR_PAIR_BORDER));
    box(bw, 0, 0);
    wattroff(bw, COLOR_PAIR(COLOR_PAIR_BORDER));

    wrefresh(bw);
}

/* ─── Sidebar window ─────────────────────────────────────────────────────── */

static void draw_sidebar(WINDOW *sw, const GameState *g)
{
    int row, col;

    werase(sw);
    wattron(sw, COLOR_PAIR(COLOR_PAIR_UI));

    /* Next piece preview */
    mvwaddstr(sw, 0, 0, "NEXT:");
    for (row = 0; row < 4; row++) {
        for (col = 0; col < 4; col++) {
            if (PIECE_SHAPES[g->next_piece][0][row][col]) {
                wattron(sw, COLOR_PAIR(PIECE_COLORS[g->next_piece]));
                mvwaddstr(sw, row + 1, col * CELL_W, "  ");
                wattroff(sw, COLOR_PAIR(PIECE_COLORS[g->next_piece]));
            }
        }
    }

    /* Stats */
    wattron(sw, COLOR_PAIR(COLOR_PAIR_UI));
    mvwprintw(sw,  6, 0, "SCORE:");
    mvwprintw(sw,  7, 0, "%-10d", g->score);
    mvwprintw(sw,  9, 0, "LEVEL:");
    mvwprintw(sw, 10, 0, "%-10d", g->level);
    mvwprintw(sw, 12, 0, "LINES:");
    mvwprintw(sw, 13, 0, "%-10d", g->lines_cleared);

    /* Controls */
    mvwaddstr(sw, 15, 0, "─ CONTROLS ─");
    mvwaddstr(sw, 16, 0, "←→  Move");
    mvwaddstr(sw, 17, 0, "↑   Rotate CW");
    mvwaddstr(sw, 18, 0, "z   Rotate CCW");
    mvwaddstr(sw, 19, 0, "↓   Soft drop");
    mvwaddstr(sw, 20, 0, "SPC Hard drop");
    mvwaddstr(sw, 21, 0, "p   Pause");
    mvwaddstr(sw, 22, 0, "q   Quit");

    if (g->paused) {
        wattron(sw, A_BOLD);
        mvwaddstr(sw, 24, 0, "*** PAUSED ***");
        wattroff(sw, A_BOLD);
    }

    wattroff(sw, COLOR_PAIR(COLOR_PAIR_UI));
    wrefresh(sw);
}

/* ─── Game-over overlay ──────────────────────────────────────────────────── */

static void draw_game_over(WINDOW *bw)
{
    int center = BOARD_HEIGHT / 2;

    wattron(bw, A_BOLD | COLOR_PAIR(COLOR_PAIR_UI));
    mvwaddstr(bw, center - 1, 1, "                    ");
    mvwaddstr(bw, center,     1, "    GAME  OVER!     ");
    mvwaddstr(bw, center + 1, 1, "  Press r to retry  ");
    mvwaddstr(bw, center + 2, 1, "                    ");
    wattroff(bw, A_BOLD | COLOR_PAIR(COLOR_PAIR_UI));

    wrefresh(bw);
}

/* ─── Main ───────────────────────────────────────────────────────────────── */

int main(void)
{
    int board_win_h, board_win_w;
    int total_w, total_h;
    int start_y, start_x;
    int term_rows, term_cols;
    int ch;
    WINDOW *board_win, *side_win;
    GameState game;

    /* ncurses init */
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    if (!has_colors()) {
        endwin();
        fprintf(stderr, "Your terminal does not support colors.\n");
        return 1;
    }
    init_colors();

    /* Terminal size check */
    getmaxyx(stdscr, term_rows, term_cols);
    board_win_h = BOARD_HEIGHT + 2;              /* +2 for border rows */
    board_win_w = BOARD_WIDTH  * CELL_W + 2;     /* +2 for border cols */
    total_w     = board_win_w + 2 + SIDEBAR_W;
    total_h     = board_win_h;

    if (term_rows < total_h || term_cols < total_w) {
        endwin();
        fprintf(stderr,
                "Terminal too small. Need at least %d×%d (current %d×%d).\n",
                total_w, total_h, term_cols, term_rows);
        return 1;
    }

    /* Center the game area */
    start_y = (term_rows - total_h) / 2;
    start_x = (term_cols - total_w) / 2;

    board_win = newwin(board_win_h, board_win_w, start_y, start_x);
    side_win  = newwin(total_h, SIDEBAR_W,       start_y, start_x + board_win_w + 2);

    /* Seed RNG once at startup so restarts get different piece sequences */
    srand((unsigned int)time(NULL));
    game_init(&game);

    /* Main game loop */
    while (1) {
        ch = getch();

        if (ch == 'q' || ch == 'Q')
            break;

        if (game.game_over) {
            if (ch == 'r' || ch == 'R') {
                game_init(&game);
                werase(board_win);
            }
        } else if (game.paused) {
            if (ch == 'p' || ch == 'P')
                game.paused = 0;
        } else {
            switch (ch) {
            case KEY_LEFT:               game_move(&game, -1,  0); break;
            case KEY_RIGHT:              game_move(&game,  1,  0); break;
            case KEY_DOWN:               game_move(&game,  0,  1); break;
            case KEY_UP:                 game_rotate(&game,  1); break;
            case 'z': case 'Z':          game_rotate(&game, -1); break;
            case ' ':                    game_hard_drop(&game);   break;
            case 'p': case 'P':          game.paused = 1;         break;
            default: break;
            }
        }

        game_tick(&game);

        draw_board(board_win, &game);
        draw_sidebar(side_win, &game);

        if (game.game_over)
            draw_game_over(board_win);

        napms(TICK_MS);
    }

    delwin(board_win);
    delwin(side_win);
    endwin();

    printf("Final score: %d  (level %d, %d lines cleared)\n",
           game.score, game.level, game.lines_cleared);
    return 0;
}
