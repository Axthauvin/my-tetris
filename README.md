# my-tetris

Tetris made in C, running in the terminal with ncurses.

## Build

```sh
make
```

Requires `gcc` and `libncurses-dev` (or equivalent on your system).

## Run

```sh
./tetris
```

Your terminal needs to be at least **44 columns × 22 rows** and must support colors.

## Controls

| Key | Action |
|-----|--------|
| `←` / `→` | Move piece left / right |
| `↑` | Rotate clockwise |
| `z` | Rotate counter-clockwise |
| `↓` | Soft drop |
| `Space` | Hard drop |
| `p` | Pause / resume |
| `r` | Restart (after game over) |
| `q` | Quit |

## Scoring

| Lines cleared | Points (× level) |
|---------------|-----------------|
| 1 | 100 |
| 2 | 300 |
| 3 | 500 |
| 4 (Tetris) | 800 |

Hard-dropping a piece also scores bonus points.  
Every 10 lines clears advances the level and increases gravity speed.
