# Quarto

A desktop version of the board game **Quarto**, written in C++20 with a 2D
[raylib](https://www.raylib.com/) front end and a built-in search engine
(negamax) to play against.

> **Status: early work in progress.** The engine, the game state (with win and
> draw detection) and a first piece-pool view are working. Most of the UI,
> match and player files are still empty stubs. See
> [docs/ui-architecture.md](docs/ui-architecture.md) for the planned design.

## The game

Quarto is played on a 4×4 board with 16 unique pieces. Each piece has four
binary properties (e.g. dark/light, tall/short, round/square, solid/hollow).

On each turn a player:

1. places the piece their opponent handed them on any empty cell, then
2. picks one of the remaining pieces and hands it to the opponent.

A player wins by completing a line of four (row, column or diagonal) whose
pieces all share at least one property. If the board fills up without such a
line, the game is a draw.

## Requirements

| What | Version | Notes |
|---|---|---|
| C++ compiler | C++20 | Developed with GCC 16 from MSYS2 UCRT64 |
| CMake | ≥ 3.20 | |
| Ninja | any | Recommended generator |
| Git | any | CMake uses it to download raylib |
| raylib | 5.5 | **Downloaded automatically** by CMake (`FetchContent`); no manual install |

The first configure needs an internet connection to clone raylib into
`build/_deps/`.

### Windows (MSYS2)

Install [MSYS2](https://www.msys2.org/), then in the *MSYS2 UCRT64* shell:

```sh
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja git
```

Add `C:\msys64\ucrt64\bin` to your Windows `PATH`.

### Linux

Install a C++20 compiler, CMake, Ninja and Git, plus the X11/OpenGL development
packages that raylib needs. For example, on Debian or Ubuntu:

```sh
sudo apt install build-essential cmake ninja-build git \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

## Build and run

From the project root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/quarto        # Windows: build\quarto.exe
```

Use `-DCMAKE_BUILD_TYPE=Debug` for debugging. If you leave out
`CMAKE_BUILD_TYPE`, CMake builds without optimisations, and the engine runs
much slower.

The game can be started from any working directory. Assets are loaded relative
to the executable (`<exe dir>/../assets/`), so **`quarto.exe` must stay in
`build/`**, one level below `assets/`. If you move the executable, copy
`assets/` alongside it so that path still resolves.

## Project structure

```
quarto-game/
├── CMakeLists.txt          build definition; fetches raylib 5.5
├── assets/
│   ├── fonts/              UI font goes here (see its README)
│   └── images/
│       ├── buttons/        button backgrounds
│       └── pieces/         piece1.png … piece16.png
├── docs/
│   └── ui-architecture.md  full UI design: layers, data flow, file roles
├── tools/
│   └── match.py            headless engine-vs-engine match runner
└── src/
    ├── main.cpp            creates Game + Gui and runs the main loop
    ├── engine/             search engine (namespace quarto)
    ├── game/               game state and rules, independent of the UI
    ├── players/            human and engine players, engine plug-ins
    ├── match/              turn flow, commands, background engine thread
    └── ui/                 everything that uses raylib
```

### Layers

The code is split into layers. A layer may only include layers **below** it:

```
ui/        App, screens, views, widgets, theme, input, animation   ← only layer using raylib
match/     MatchController, Commands, EngineWorker, MatchRunner
players/   Player, HumanPlayer, EnginePlayer, engines/ (IEngine, registry, adapters)
game/      Game state, Move, rules, engine_bridge
engine/    search
```

- `game/`, `players/`, `match/` and `engine/` never include raylib, so they can
  be unit-tested and run headless.
- Only `players/engines/` and `game/engine_bridge` include `engine/` headers.
- Views read state but never change it.

### Key directories

| Path | Contents |
|---|---|
| `src/engine/` | Bitboard `Position` (Zobrist hashing, 17-entry undo history), `wincheck` (fast line detection), `search` (negamax with alpha-beta). Public entry point: `quarto::find_best_move(grid, piece_in_hand, available, depth)` in `search.hpp`. |
| `src/game/` | `Game` class: board, piece in hand, available pieces, undo history, win/draw detection. `types.hpp` holds `Piece`, `Cell`, `Board`, `Move`. |
| `src/ui/app.*` | `Gui`: opens the window and runs the draw loop. |
| `src/ui/layout.hpp` | Layout constants in a **1920×1080 design space**, plus `ui_scale()` to map them to the real window size. |
| `src/ui/screens/` | Game screen (piece pool), main menu, results. |
| `src/ui/widgets/` | Button, dropdown, panel, scroll list, slider. |

### Data encoding

- **Cells** are `0..15`, index = `row * 4 + col`. `-1` means none.
- **Pieces** are `0..15`, and the four bits of the number are the piece's four
  properties. `-1` means none. Two pieces share a property when they have the
  same value in that bit.
- A **move** is always a pair: `{cell to place on, piece to give}`.

## Engine matches

`tools/match.py` plays engines against each other without the GUI. It needs
Python 3 and the engines built in `build/`:

```sh
python tools/match.py --a thor --b odin --games 100 --movetime 200
```

Games start from the empty board, and the engines take turns moving first.
The runner checks every move against the rules, then prints the result of each
game with the seeds both engines got, followed by the score and an Elo estimate.

## Development notes

- **New `.cpp` files must be added to `add_executable` in `CMakeLists.txt`.**
  Otherwise the build fails at the link step with "undefined reference" errors.
  The empty stub files are left out on purpose until they contain code.
- Every header needs `#pragma once` as its first line. Without it, headers that
  get included twice cause "redefinition" errors.
- Don't call raylib functions such as `GetScreenWidth()` in global or `static`
  initialisers. They run before `InitWindow()` and return 0.
- Load assets with `GetApplicationDirectory()` (see `game_screen.cpp`), not
  with a bare relative path. A relative path only works when the game is
  started from one specific folder.
- On Windows, close the running game before rebuilding. Otherwise the linker
  fails with `cannot open output file quarto.exe: Permission denied`.
- `build/` is git-ignored.
