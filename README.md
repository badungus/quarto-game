# Quarto

A desktop version of the board game **Quarto**, written in C++20 with a
[raylib](https://www.raylib.com/) GUI and four engines to play against, from a
plain alpha-beta search to a perfect-play solver with an opening book.

![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-%E2%89%A5%203.20-064F8C?logo=cmake&logoColor=white)
![raylib](https://img.shields.io/badge/raylib-5.5-black)
![Platforms](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey)
[![License: MIT](https://img.shields.io/badge/license-MIT-green)](LICENSE)

## Contents

- [Features](#features)
- [The game](#the-game)
- [Engines](#engines)
- [Getting started](#getting-started)
- [Usage](#usage)
- [UQI protocol](#uqi-protocol)
- [Headless engine matches](#headless-engine-matches)
- [Project structure](#project-structure)
- [Architecture](#architecture)
- [Development notes](#development-notes)
- [License](#license)

## Features

- **Desktop GUI** built with raylib. It opens at the monitor's resolution and
  scales a 1920×1080 layout to fit.
- **Any pairing of players**: human vs human, human vs engine or engine vs
  engine. Each seat can be the human player or any of the four engines.
- **Per-player engine settings** in the main menu: search depth, move time and
  infinite search.
- **Game controls**: undo, redo, restart, pause / resume and stop search.
- **Engine insight**: an evaluation bar for each player and a scrollable view of
  the engine's live search output, grouped by move.
- **Engine Match screen**: play a series of games between two engines from the
  GUI and get the score and an Elo estimate.
- **Engines run as separate processes** and talk to the GUI over a small text
  protocol ([UQI](#uqi-protocol)), so an engine can be developed, tested and
  replaced without touching the GUI.
- **Headless match runner** in Python for engine-vs-engine testing without the
  GUI.

## The game

Quarto is played on a 4×4 board with 16 unique pieces. Each piece has four
binary properties (e.g. dark/light, tall/short, round/square, solid/hollow).

On each turn a player:

1. places the piece their opponent handed them on any empty cell, then
2. picks one of the remaining pieces and hands it to the opponent.

The first player starts by only handing over a piece. A player wins by
completing a line of four (row, column or diagonal) whose pieces all share at
least one property. If the board fills up without such a line, the game is a
draw.

## Engines

All engines live in `src/engine/<name>/`, build to their own executable and
speak UQI on stdin/stdout.

| Engine      | Approach |
|-------------|----------|
| **Baldur**  | Baseline: iterative-deepening negamax with alpha-beta pruning on a bitboard position, root moves searched with a full window. |
| **Thor**    | Baldur plus a check for an immediate win before any subtree is searched, a narrowed root window that cuts off worse moves early, and win scores based on the number of pieces on the board. |
| **Odin**    | Thor plus a transposition table (exact keys, packed 16-byte entries, generations), symmetry reduction (32 board symmetries × 384 piece relabelings) for shallow positions, and history-based move ordering. |
| **Optimal** | Perfect play. Plays from an opening book (`src/engine/optimal/book.txt`) while the position is in it, and solves the position to the end of the game after that. Ignores depth and move-time limits. The book shows the start position is a draw. |

Baldur, Thor and Odin accept `depth`, `movetime` and `infinite` limits and
report a forced win or loss as `mate N` once they find one.

### Optimal on the command line

Besides running as a UQI engine, `optimal` can solve positions directly:

```sh
optimal --solve POSITION [--tt MB]          # prove win / draw / loss for one position
optimal --full [--threads N] [--tt MB]      # solve the whole game and write the book
```

- `--solve` runs on one thread without the book and prints the result for the
  side to move, a move that achieves it, the node count and the time.
- `--full` solves the empty board on `N` threads (default: one per hardware
  thread) and writes every position of the first few turns to `book.txt`. It
  takes hours; finished jobs are saved as they complete, so an interrupted run
  picks up where it stopped.
- `--tt` sets the transposition table size in MB (default 8192).

## Getting started

### Requirements

| What | Version | Notes |
|---|---|---|
| C++ compiler | C++20 | Developed with GCC from MSYS2 UCRT64 |
| CMake | ≥ 3.20 | |
| Ninja | any | Recommended generator |
| Git | any | CMake uses it to download the dependencies |
| raylib | 5.5 | **Downloaded automatically** by CMake (`FetchContent`) |
| tiny-process-library | pinned commit | **Downloaded automatically**; launches the engine processes |
| Python | 3 | Only for `tools/match.py` |

The first configure needs an internet connection to clone the dependencies into
`build/_deps/`.

#### Windows (MSYS2)

Install [MSYS2](https://www.msys2.org/), then in the *MSYS2 UCRT64* shell:

```sh
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja git
```

Add `C:\msys64\ucrt64\bin` to your Windows `PATH`. Without it the compiler can
fail silently.

#### Linux

Install a C++20 compiler, CMake, Ninja and Git, plus the X11/OpenGL development
packages that raylib needs. For example, on Debian or Ubuntu:

```sh
sudo apt install build-essential cmake ninja-build git \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

### Build

From the project root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This builds the GUI (`quarto`) and all four engines (`baldur`, `thor`, `odin`,
`optimal`) into `build/`. Use `-DCMAKE_BUILD_TYPE=Debug` for debugging. Without
a build type, CMake builds without optimisations and the engines run much
slower.

### Run

```sh
./build/quarto        # Windows: build\quarto.exe
```

The game can be started from any working directory. It loads assets from
`<exe dir>/../assets/` and launches the engines from its own directory, so
**keep `quarto` and the engine executables together in `build/`**, one level
below `assets/`.

## Usage

1. In the main menu, choose **Player One** and **Player Two**: `PLAYER` for a
   human, or one of the engines.
2. For engine seats, adjust **DEPTH** (default 16), **MOVETIME** in
   milliseconds (default 3000) and **INFINITE** below the player's dropdown. A
   cleared field is not sent to the engine.
3. Press **Start**. A human player places the piece in hand by clicking an
   empty board cell, then picks the piece to hand over from the pool.
4. Use the buttons on the game screen to undo, redo, restart, pause or resume
   the game, or to stop the engine's current search.

The **Engine Match** button opens a screen that plays a series of games between
two engines, with the number of games, a seed and per-engine settings, and
shows the score and an Elo estimate when it finishes.

## UQI protocol

The GUI and the engines communicate over stdin/stdout with UQI, a small
line-based protocol modelled on chess's UCI.

### Commands (GUI → engine)

| Command | Meaning |
|---|---|
| `uqi` | Handshake; the engine replies `uqiok`. |
| `isready` | The engine replies `readyok`. |
| `seed N` | Fixes the engine's tie-break random generator until the next `seed`. |
| `go [depth N] [movetime MS] [infinite] [position POS]` | Sets the limits and position and starts a search. |
| `stop` | Stops the current search; the engine still sends `bestmove`. |
| `quit` | Stops the search and exits. |

### Output (engine → GUI)

| Line | Meaning |
|---|---|
| `info depth D score S [mate N] nodes K move M` | Result of a completed search depth. `mate N > 0` wins on the engine's own N-th turn, `N < 0` loses on the opponent's N-th turn. Optimal appends `book` when the move came from the opening book. |
| `info seed N` | The seed in use, so a game can be replayed. |
| `bestmove M` | The move the engine plays. |

### Notation

**Pieces** are four letters, one per property bit, in the order `c h s f`. An
uppercase letter means the bit is set: `chsf` is piece 0, `CHSF` is piece 15.

**Positions** list the four ranks from top to bottom, separated by `/`. Each
cell is `.` when empty or a piece. After `|` comes the piece in hand, or `-`
when there is none (the start of the game):

```
..../..../..../....|-                       empty board
ChSfchsf../ChSF..chSF/..../...chsF|CHSF    five pieces placed, CHSF in hand
```

**Moves** are `CELL|PIECE`. The cell is `a1`–`d4` (files `a`–`d` left to right,
ranks `1`–`4` bottom to top) or `-` for the opening give-only move. The piece is
the one handed to the opponent, or `#` when the placement wins the game.

### Example session

Thor with a fixed seed (`>` is sent to the engine, `<` is its reply):

```
> uqi
< uqiok
> seed 1
> go depth 4 position ..../..../..../....|-
< info seed 1
< info depth 1 score 0 nodes 17 move -|chsF
< info depth 2 score 0 nodes 3874 move -|cHSF
< info depth 3 score 0 nodes 14915 move -|cHsF
< info depth 4 score 0 nodes 729428 move -|cHSF
< bestmove -|cHSF
> go movetime 500 position ..../..../..../chsF...|CHSF
< info depth 1 score 0 nodes 211 move a2|cHsf
< info depth 2 score 0 nodes 38642 move c1|CHSf
< info depth 3 score 0 nodes 152727 move c1|cHSf
< bestmove c1|cHSf
> quit
```

## Headless engine matches

`tools/match.py` plays two engines against each other without the GUI. It needs
Python 3 and the engines built in `build/`:

```sh
python tools/match.py --a thor --b odin --games 100 --movetime 200
```

| Option | Default | Meaning |
|---|---|---|
| `--a` | `thor` | First engine (executable name) |
| `--b` | `odin` | Second engine (executable name) |
| `--games` | `100` | Number of games |
| `--movetime` | `200` | Milliseconds per move |
| `--seed` | `1` | Seeds the per-game engine seeds |
| `--build` | `build/` | Directory with the engine executables |

Games start from the empty board and the engines take turns moving first. The
runner checks every move against the rules (an illegal move loses), prints each
game with the seeds both engines got, and finishes with the score, first- and
second-player wins, average game length and an Elo estimate with a 95%
confidence interval.

## Project structure

```
quarto-game/
├── CMakeLists.txt            build definition; fetches raylib and tiny-process-library
├── LICENSE
├── assets/images/
│   ├── board/                board background, winning-cell highlight
│   ├── buttons/              button backgrounds and icons
│   ├── pieces/               piece0.png … piece15.png
│   └── players/              evaluation bar
├── tools/
│   └── match.py              headless engine-vs-engine match runner
└── src/
    ├── main.cpp              creates the game and GUI and runs the main loop
    ├── engine/               standalone UQI engines, one executable each
    │   ├── baldur/
    │   ├── thor/
    │   ├── odin/
    │   └── optimal/          solver + book.txt opening book
    ├── game/                 game state and rules, independent of the UI
    ├── match/                match configuration, turn flow, commands
    ├── players/              human and engine players
    └── ui/                   everything that uses raylib
        ├── screens/          main menu, game, engine match
        ├── widgets/          button, dropdown, text field
        └── theme/            asset loading and theme
```

## Architecture

The GUI is split into layers. A layer may only include layers **below** it:

```
ui/        App, screens, widgets, theme, input     ← only layer using raylib
match/     MatchController, commands, MatchConfig
players/   Player, HumanPlayer, EnginePlayer
game/      Game state, Move, rules
```

- `game/`, `players/` and `match/` never include raylib, so they can run
  headless.
- The GUI never links an engine. `EnginePlayer` starts the engine executable as
  a child process and talks to it over UQI.
- Screens read the game state but never change it directly: user actions
  become commands (`src/game/command.hpp`) that the match controller applies.

### Data encoding

- **Cells** are `0..15`, index = `row * 4 + col`. `-1` means none.
- **Pieces** are `0..15`, and the four bits of the number are the piece's four
  properties. `-1` means none. Two pieces share a property when they have the
  same value in that bit.
- A **move** is always a pair: `{cell to place on, piece to give}`.

## Development notes

- **Adding an engine**: create `src/engine/<name>/` with its own
  `CMakeLists.txt`, add `add_subdirectory(src/engine/<name>)` to the root
  `CMakeLists.txt`, and add an entry to `ENGINES` in
  `src/game/match_config.hpp`.
- **New GUI `.cpp` files must be added to `add_executable` in
  `CMakeLists.txt`**, otherwise the link step fails with "undefined reference"
  errors.
- Every header starts with `#pragma once`.
- Don't call raylib functions such as `GetScreenWidth()` in global or `static`
  initialisers. They run before `InitWindow()` and return 0.
- Load assets with `GetApplicationDirectory()` (see `game_screen.cpp`), not
  with a bare relative path, which only works from one specific folder.
- On Windows, close the running game before rebuilding. Otherwise the linker
  fails with `cannot open output file quarto.exe: Permission denied`.

## License

This project is licensed under the [MIT License](LICENSE).
