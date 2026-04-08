#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#define NB_ROWS 20
#define NB_COLS 10
#define REFRESH_MS 500
#define BORDER_WIDTH 1

#define true 1
#define false 0

const char WALL = 'X';

void sleepms(int ms)
{
    usleep(ms * 1000);
    // sleep(1);
}

void set_nonblocking(int enable)
{
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);

    if (enable)
    {
        ttystate.c_lflag &= ~ICANON;
        ttystate.c_lflag &= ~ECHO;
    }
    else
    {
        ttystate.c_lflag |= ICANON;
        ttystate.c_lflag |= ECHO;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

int kbhit()
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

void clear_screen()
{
    printf("\033[2J\033[H");
    fflush(stdout);
}

char BOARD[NB_ROWS + BORDER_WIDTH * 2][NB_COLS + BORDER_WIDTH * 2];
size_t score = 0;

void init_board()
{
    for (int i = 0; i < NB_ROWS + BORDER_WIDTH * 2; i++)
    {
        for (int j = 0; j < NB_COLS + BORDER_WIDTH * 2; j++)
        {
            if (i == 0 || i == NB_ROWS + BORDER_WIDTH || j == 0
                || j == NB_COLS + BORDER_WIDTH)
                BOARD[i][j] = WALL;
            else
                BOARD[i][j] = ' ';
        }
    }
}

struct piece
{
    size_t x;
    size_t y;
    size_t lenx;
    size_t leny;
    char **SHAPES;
} typedef piece;

void move_piece(piece *piece, size_t x, size_t y)
{
    if (x + piece->lenx - 1 > NB_COLS || y + piece->leny - 1 > NB_ROWS)
        return;

    // collision detection
    // for the size of piece, check each cell with the target y if its a [ or ]
    for (size_t i = 0; i < piece->lenx; i++)
    {
        for (size_t j = 0; j < piece->leny; j++)
        {
            if (piece->SHAPES[j][i] != ' ' && BOARD[y + j][x + i] != ' ')
            {
                // printf("collusion detected at x: %zu, y: %zu\n", x + i, y +
                // j);
                return;
            }
        }
    }

    piece->x = x;
    piece->y = y;
}

void rotate_piece(piece *piece)
{
    // rotate the piece 90 degrees clockwise
    char **new_shape = malloc(sizeof(char *) * piece->lenx);
    for (size_t i = 0; i < piece->lenx; i++)
    {
        new_shape[i] = malloc(sizeof(char) * piece->leny);
        for (size_t j = 0; j < piece->leny; j++)
        {
            new_shape[i][j] = piece->SHAPES[piece->leny - j - 1][i];
        }
    }

    // free old shape
    // for (size_t i = 0; i < piece->leny; i++)
    //     free((void *)piece->SHAPES[i]);
    // free((void *)piece->SHAPES);

    piece->SHAPES = new_shape;

    // swap lenx and leny
    size_t temp = piece->lenx;
    piece->lenx = piece->leny;
    piece->leny = temp;
}

void add_piece_to_board(piece *piece)
{
    for (size_t i = 0; i < piece->leny; i++)
    {
        for (size_t j = 0; j < piece->lenx; j++)
        {
            if (piece->SHAPES[i][j] != ' ')
                BOARD[piece->y + i][piece->x + j] = piece->SHAPES[i][j];
        }
    }
}

void game(piece PIECE)
{
    for (int i = 0; i < NB_ROWS + BORDER_WIDTH * 2; i++)
    {
        for (int j = 0; j < NB_COLS + BORDER_WIDTH * 2; j++)
        {
            // putchar(BOARD[i][j]);
            // printf("current piece x: %zu, y: %zu\n", PIECE.x, PIECE.y);
            // printf("coords i: %i, j: %i\n", i, j);
            // printf("test: %i, %i\n", (int)(i - PIECE.y), (int)(j - PIECE.x));

            if ((int)(i - PIECE.y) >= 0 && (int)(j - PIECE.x) >= 0
                && (int)(i - PIECE.y) < PIECE.leny
                && (int)(j - PIECE.x) < PIECE.lenx
                && PIECE.SHAPES[i - PIECE.y][j - PIECE.x] != ' ')
                putchar(PIECE.SHAPES[i - PIECE.y][j - PIECE.x]);
            else
                putchar(BOARD[i][j]);
        }
        if (i == 0)
            printf("                                  Score : %li", score);
        putchar('\n');
    }
}

const char *SHAPE1[] = {
    "## ",
    " ##",
};

piece PIECE1 = {
    .x = 1,
    .y = 1,
    .lenx = 3,
    .leny = 2,
    .SHAPES = SHAPE1,
};

const char *SHAPE2[] = {
    "##",
    "##",
};

piece PIECE2 = {
    .x = 1,
    .y = 1,
    .lenx = 2,
    .leny = 2,
    .SHAPES = SHAPE2,
};

const char *SHAPE3[] = {
    "####",
};

piece PIECE3 = {
    .x = 1,
    .y = 1,
    .lenx = 4,
    .leny = 1,
    .SHAPES = SHAPE3,
};

const char *SHAPE4[] = {
    "###",
    " # ",
};

piece PIECE4 = {
    .x = 1,
    .y = 1,
    .lenx = 3,
    .leny = 2,
    .SHAPES = SHAPE4,
};

piece *create_piece()
{
    piece *PIECE = malloc(sizeof(piece));
    // pick a random piece
    int r = rand() % 4;
    switch (r)
    {
    case 0:
        memccpy(PIECE, &PIECE1, sizeof(piece), sizeof(piece));
        break;
    case 1:
        memccpy(PIECE, &PIECE2, sizeof(piece), sizeof(piece));
        break;
    case 2:
        memccpy(PIECE, &PIECE3, sizeof(piece), sizeof(piece));
        break;
    case 3:
        memccpy(PIECE, &PIECE4, sizeof(piece), sizeof(piece));
        break;
    }

    return PIECE;
}

int check_full_line()
{
    for (int i = BORDER_WIDTH; i < NB_ROWS + BORDER_WIDTH; i++)
    {
        int is_line = i;
        for (int j = 0; j < NB_COLS + BORDER_WIDTH * 2; j++)
        {
            if (BOARD[i][j] == ' ')
                is_line = false;
        }

        if (is_line)
            return i;
    }

    return false;
}

void remove_line(int line)
{
    for (int i = line; i > 0; i--)
    {
        for (int j = 0; j < NB_COLS + BORDER_WIDTH * 2; j++)
        {
            BOARD[i][j] = BOARD[i - 1][j];
        }
    }

    // clear the top line
    for (int j = 0; j < NB_COLS + BORDER_WIDTH * 2; j++)
    {
        BOARD[BORDER_WIDTH][j] = ' ';
    }
}

int main(void)
{
    struct timeval stop, start;
    set_nonblocking(1);
    gettimeofday(&start, NULL);
    piece *PIECE = create_piece();

    size_t x = NB_COLS / 2 - PIECE->lenx / 2 + BORDER_WIDTH;
    size_t y = BORDER_WIDTH;
    init_board();

    move_piece(PIECE, x, y);
    game(*PIECE);

    int now = 0;

    while (true)
    {
        if (kbhit())
        {
            char c = getchar();
            if (c == 65)
            {
                // printf("up\n");
                rotate_piece(PIECE);
            }

            if (c == 66)
            {
                // printf("down\n");
                now = true;
            }

            if (c == 67)
            {
                // printf("right\n");
                x = (x < NB_COLS - PIECE->lenx + BORDER_WIDTH) ? x + 1 : x;
                move_piece(PIECE, x, y);
            }

            if (c == 68)
            {
                // printf("left\n");
                x = (x > BORDER_WIDTH) ? x - 1 : x;
                move_piece(PIECE, x, y);
            }

            if (c == 'c')
                break;
        }

        gettimeofday(&stop, NULL);
        long elapsed_us = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec
            - start.tv_usec;
        if (elapsed_us >= REFRESH_MS * 1000 || now)
        {
            now = 0;
            // printf("tick\n");
            // printf("took %ld us\n", elapsed_us);
            start = stop;
            clear_screen();
            size_t old_y = y;
            y++;
            move_piece(PIECE, x, y);
            if (old_y == PIECE->y)
            {
                // piece has reached the bottom, create a new one
                add_piece_to_board(PIECE);
                free(PIECE);

                int full_line = check_full_line();
                if (full_line)
                {
                    score += 100;
                    remove_line(full_line);
                }

                PIECE = create_piece();
                x = NB_COLS / 2 - PIECE->lenx / 2 + BORDER_WIDTH;
                y = BORDER_WIDTH;
                move_piece(PIECE, x, y);
            }
            game(*PIECE);
        }
    }

    free(PIECE);

    set_nonblocking(0);
    return 0;
}
