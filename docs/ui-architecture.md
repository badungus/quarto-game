# Quarto UI Architecture

This document sets the structure of the Quarto desktop UI: a 2D, stylized raylib front end
for the search engine in `src/engine/`. Every file in the tree below already exists as a stub
whose header comment states its job. Implementation fills them in without changing the shape.

---

## 1. Goals and non-goals

**Goals**

- **Looks polished.** Consistent theme, crisp vector-drawn pieces, smooth feedback and animation.
- **Strict layering.** Rules and game state know nothing about rendering. Rendering never mutates state.
- **Pluggable engines.** New engines can be added and compared against each other with no UI changes.
- **Never freezes.** Engine searches run off the main thread, and the UI stays at 60 fps while they run.

**Features in scope**

| Feature | Summary |
|---|---|
| Human vs engine | Pick an engine and its settings, and pick who moves first |
| Engine vs engine | Watch a game live, or run an N-game series (with UI or headless) to compare engines |
| Undo / redo and move history | Move list panel, undo/redo buttons, click a move to jump to it |
| Analysis panel | Engine score, depth, nodes, nps, best-move hint, thinking indicator |

**Non-goals:** 3D rendering, networking, online play, mobile.

---

## 2. Layered architecture

```
┌──────────────────────────────────────────────────────────────┐
│ ui/        App + main loop, screens, views, widgets,         │  ← only layer that
│            piece renderer, theme, input, animation           │    includes raylib
├──────────────────────────────────────────────────────────────┤
│ match/     MatchController, Commands, EngineWorker,          │
│            MatchRunner (N-game series, headless)             │
├──────────────────────────────────────────────────────────────┤
│ players/   Player, HumanPlayer, EnginePlayer                 │
│            engines/: IEngine, EngineRegistry, adapters       │
├──────────────────────────────────────────────────────────────┤
│ game/      GameState, Move, rules, History, engine_bridge    │  ← pure C++,
│                                                              │    unit-testable
├──────────────────────────────────────────────────────────────┤
│ engine/    existing search (unchanged)                       │
└──────────────────────────────────────────────────────────────┘
```

**Dependency rules**

1. A layer may include only the layers below it, never the ones above.
2. `game/`, `players/`, `match/` and `engine/` never include raylib. This keeps them testable and lets `MatchRunner` run headless.
3. Only `players/engines/` and `game/engine_bridge` include `src/engine/` headers. Everything else talks to engines through `IEngine`.
4. Views **read** state and never write it. Every change goes through `MatchController::apply(Command)`.

---

## 3. Directory layout

```
docs/
  ui-architecture.md             this document
assets/
  fonts/                         UI font(s), copied next to the binary at build time
src/
  main.cpp                       entry point: parse CLI flags, run App (or headless MatchRunner)
  engine/                        existing search engine (untouched)

  game/
    move.hpp                     Move {cell, give}; mirrors quarto::Move with value semantics
    game_state.hpp/.cpp          board, piece in hand, available pieces, side to move, phase, result
    rules.hpp/.cpp               legality checks, applying a move, win/draw detection, winning line
    history.hpp/.cpp             undo/redo stack of moves; rebuild any ply by replay
    engine_bridge.hpp/.cpp       GameState -> quarto::Grid / piece_in_hand / available; engine move back

  players/
    player.hpp                   Player interface: human or engine, name, kind
    human_player.hpp/.cpp        player whose moves come from UI input
    engine_player.hpp/.cpp       player that owns an IEngine instance and its options
    engines/
      search_types.hpp           SearchRequest, SearchResult, SearchStats, StopToken, EngineOption
      iengine.hpp                IEngine interface: name, options, search
      engine_registry.hpp/.cpp   name -> factory table; lists engines to the setup screen
      minimax_engine.hpp/.cpp    adapter over quarto::find_best_move

  match/
    command.hpp                  Command variant: PlacePiece, GivePiece, Undo, Redo, JumpTo, NewGame, Pause, Step...
    match_controller.hpp/.cpp    owns GameState + History + 2 Players; applies commands; drives turns
    engine_worker.hpp/.cpp       runs one search on a background thread; mailbox + generation id
    match_runner.hpp/.cpp        engine-vs-engine series: N games, alternate starts, tally results

  ui/
    app.hpp/.cpp                 window setup, main loop, screen stack, Theme/Assets lifetime
    layout.hpp                   Rect helpers: split, inset, center, aspect-fit
    input.hpp/.cpp               per-frame input snapshot (mouse, keys, hovered/active widget)
    screens/
      screen.hpp                 Screen interface: update(input), draw(), transition requests
      main_menu_screen.*         title, New Game, Engine Match, Quit
      match_setup_screen.*       choose players/engines, engine options, first mover, series length
      game_screen.*              composes all views; turns input into Commands
      results_screen.*           game/series result, W/D/L tally, rematch / back
    views/
      board_view.*               4x4 board, placed pieces, hover ghost, winning-line highlight
      piece_pool_view.*          16-piece grid of available/used pieces; selecting the piece to give
      hand_slot_view.*           the piece currently in hand, prominent
      move_list_view.*           scrollable numbered move list; click to jump
      analysis_view.*            score bar, depth, nodes, nps, time, best-move hint, thinking spinner
      status_bar.*               player names, side to move, phase prompt
    widgets/
      button.*  dropdown.*  slider.*  panel.*  scroll_list.*    reusable immediate-mode controls
    render/
      piece_renderer.hpp/.cpp    the ONE function that draws a Quarto piece in a rect
    theme/
      theme.hpp/.cpp             palette, spacing scale, radii, font sizes, animation durations
      assets.hpp/.cpp            load/unload fonts once; handles owned by App
    anim/
      tween.hpp                  small easing/tween helpers for placement and give animations
```

(`name.*` means `name.hpp` + `name.cpp`.)

---

## 4. Game model (`game/`)

The model represents the full Quarto turn explicitly:

```
      ┌─────────── opening only ───────────┐
      ▼                                    │
  [Give] ──► opponent's turn ──► [Place] ──► [Give] ──► ...
                                    │
                                    └─► win / board full → GameOver
```

- **`GameState`**
  - `board`: `std::array<std::optional<Piece>, 16>`, indexed `row * 4 + col`, the same as the engine's `Cell`.
  - `hand`: the piece the side to move must place. It is `nullopt` at the opening.
  - `available`: the pieces not placed and not in hand.
  - `side`, `phase` (`Place` / `Give` / `GameOver`), `result` (`Ongoing` / `Win(side, line)` / `Draw`).
- **`Move {cell, give}`** is one complete ply, the same as `quarto::Move`. `cell = -1` is the give-only opening ply, and `give = -1` means nothing is left to give.
- **Humans play in two steps** (click a cell, then click a pool piece). The controller buffers the half-move and commits a full `Move` to `History`. Engines produce a full `Move` in one step.
- **`rules`** reuses `quarto::move_wins` and `quarto::LINES` (`src/engine/wincheck.hpp`, `tables.hpp`) for win detection and to find the winning line so it can be highlighted.
- **`History`** is a vector of `Move` plus a cursor. Undo moves the cursor back and redo moves it forward. `JumpTo(ply)` rebuilds the state by replaying from the start. At ≤ 16 plies, replay costs nothing.
- **`engine_bridge`** is the only translation point to the engine API:
  - `GameState` → `quarto::Grid` (`vector<vector<optional<int>>>`), `optional<int> piece_in_hand`, `vector<int> available`.
  - `find_best_move`'s result `{{row, col}, give}` → `Move{row * 4 + col, give}`, with `{-1,-1}` mapped to `cell = -1`.

---

## 5. Players and pluggable engines (`players/`)

```cpp
// sketch only; real declarations go in search_types.hpp / iengine.hpp
struct EngineOption  { std::string key, label; int min, max, value; };   // e.g. depth 1..16
struct SearchRequest { GameState state; std::vector<EngineOption> options; };
struct SearchStats   { int depth; uint64_t nodes; double seconds; };
struct SearchResult  { Move move; std::optional<int> score; SearchStats stats;
                       std::vector<std::pair<Move,int>> root_scores; };   // for the analysis panel

class IEngine {
public:
    virtual ~IEngine() = default;
    virtual std::string_view name() const = 0;
    virtual std::vector<EngineOption> default_options() const = 0;
    virtual SearchResult search(const SearchRequest&, std::stop_token) = 0;
};
```

- **`EngineRegistry`** maps names to factories. `MatchSetupScreen` lists `registry.names()` and builds option controls from `default_options()`. Adding an engine takes one new `xxx_engine.cpp` and one registration line. The UI does not change.
- **`Player`** is either `HumanPlayer` or `EnginePlayer` (which owns an `IEngine` and its option values). A match is two player slots, so human vs engine, engine vs engine and human vs human all run through the same code path.
- **Current engine gaps.** These are adapter concerns. The engine itself is not changed by this plan:
  - `find_best_move` returns no score or stats, and `stats.hpp` is empty. Until the engine exposes them, the adapter fills in `stats.seconds` by timing the call, and the analysis panel shows "—" for the fields it doesn't have.
  - It has no cancellation flag. A stale search runs to completion and its result is thrown away (§6).
  - It returns `{row, col}` rather than a cell index. `engine_bridge` converts it.

---

## 6. Threading model

```
 main thread (60 fps)                         worker thread
 ───────────────────                          ─────────────
 MatchController: engine to move?
   └─► EngineWorker::start(request, gen=7) ──► engine.search(request, stop_token)
 each frame: worker.poll()  ◄──── mailbox ◄── SearchResult{gen=7}
   gen == current? apply : discard
```

- `EngineWorker` owns one `std::jthread`. It starts one search at a time and posts the result into a mutex-guarded mailbox (`std::optional<SearchResult>`).
- The main thread **polls** every frame and never joins or waits. The UI shows the thinking spinner while a job is in flight.
- Every job carries a **generation id**. Undo, new game, a jump in the history or leaving the screen bumps the generation and requests a stop. A result that arrives late with an old id is dropped.
- Nothing is shared by reference with the worker. The request holds a *copy* of the `GameState`.

---

## 7. Controller and data flow

Data flows one way:

```
 raw input ─► View hit-test ─► Command ─► MatchController::apply ─► GameState/History
                                                                        │
     draw() ◄──────────── views read const GameState& ◄─────────────────┘
```

- **Commands:** `SelectCell`, `SelectGive`, `Undo`, `Redo`, `JumpTo(ply)`, `NewGame`, `Pause`, `Resume`, `Step`, `ForceEngineMove`.
- **Human vs engine undo** rolls back to the last position where the *human* was to move. It skips the engine's reply and cancels any pending search.
- **Engine vs engine playback** has pause/resume, single-step, and a per-move delay slider (0–2 s) so games can be followed.
- `MatchController` sends out read-only events (`MovePlayed`, `GameOver`, `SearchStarted/Finished`) that views use to trigger animations. Views never poll the engine directly.

---

## 8. Screens and layout

**Screen state machine**

```
 MainMenu ──► MatchSetup ──► Game ──► Results
    ▲             ▲           │ ▲        │
    └─────────────┴───────────┘ └────────┘  (rematch / next game in series)
```

**Game screen wireframe** (reference size 1280×800; everything scales from the window size):

```
┌────────────────────────────────────────────────────────────────────────┐
│ ● Human (you)        vs        Minimax d5 ○      Pick a piece to give  │  status_bar
├───────────────────────────────────────────────┬────────────────────────┤
│                                               │  IN HAND   ┌────┐      │  hand_slot_view
│        ┌─────┬─────┬─────┬─────┐              │            │ ◉  │      │
│        │     │  ■  │     │  ●  │              │            └────┘      │
│        ├─────┼─────┼─────┼─────┤              │  PIECES                │  piece_pool_view
│        │  ◯  │     │     │     │              │  ● ○ ■ □ ◉ ◎ ▣ ▢       │
│        ├─────┼─────┼─────┼─────┤              │  · · ■ □ ◉ · ▣ ▢       │
│        │     │     │  ▣  │     │              ├────────────────────────┤
│        ├─────┼─────┼─────┼─────┤              │  MOVES                 │  move_list_view
│        │     │  ◎  │     │     │              │  1. — / give 7         │
│        └─────┴─────┴─────┴─────┘              │  2. c3 / give 12  ◄    │
│                 board_view                    ├────────────────────────┤
│                                               │  ANALYSIS   ⟳ thinking │  analysis_view
│                                               │  eval +0.0  depth 5    │
│                                               │  nodes 1.2M  nps 4.1M  │
│                                               │  hint: b2, give 3      │
├───────────────────────────────────────────────┴────────────────────────┤
│   [↶ Undo]  [↷ Redo]  [New Game]  [⏸ Pause]  [Flip]      delay ▬▬●▬▬   │  bottom bar
└────────────────────────────────────────────────────────────────────────┘
```

- The layout is computed **every frame** from `GetScreenWidth/Height()` with `layout.hpp` helpers (`split_left(rect, 0.62)`, `inset(rect, pad)`, `aspect_fit_square(rect)`). Nothing is hard-coded, and resizing just works.
- The sidebar has a minimum width. Below a certain window width it collapses the analysis panel into a tab.
- Window flags: `FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT`.

---

## 9. Visual design

**Piece encoding.** Each of the 4 property bits maps to exactly one visual channel, so every piece can be told apart at a glance:

| Bit | Property | Visual |
|---|---|---|
| 0 | tall / short | outer size (100% / 70% of slot) |
| 1 | dark / light | fill colour (`theme.piece_dark` / `theme.piece_light`) |
| 2 | round / square | circle / rounded square |
| 3 | hollow / solid | inner cut-out in background colour / none |

Bit assignment must match the engine's `prop[0..3]`. Any agreed mapping works, but it is fixed in one place (`piece_renderer.cpp`).

**`PieceRenderer::draw(Piece, Rectangle, PieceVisual state)`** is the only code that draws a piece. The board, pool, hand slot, move list and animations all call it. States:

- `Normal`
- `Hover`: slight lift, meaning a shadow offset
- `Selected`: accent outline
- `Ghost`: 40% alpha preview on the board
- `Dimmed`: already used, shown in the pool
- `Winning`: pulse

**Theme** (`theme/theme.hpp`) is the single source of style:

- **Palette:** background, surface, surface raised, border, text, text muted, accent, success, danger, piece_dark, piece_light.
- **Spacing scale:** 4 / 8 / 12 / 16 / 24 / 32. **Radii:** 6 / 10 / 16. **Font sizes:** 14 / 18 / 24 / 36.
- A dark theme by default, and a light theme defined from the same tokens.
- One font family (e.g. Inter), loaded once at 2× size with bilinear filtering so it stays crisp on HiDPI.

**Feedback and motion** (all durations in `Theme`, ~120–200 ms, ease-out):

- Hovering an empty cell shows a ghost of the piece in hand. Hovering a pool piece lifts it.
- A placement animates from the hand slot to the cell. A give animates from the pool to the hand slot.
- The winning line gets a glowing highlight and the other pieces dim.
- The engine-thinking spinner appears in the analysis panel. Human input on the board is disabled while the engine is to move.

**Style rules:** rounded panels on `surface`, a single accent colour for interaction, no pure black or white, and a subtle shadow only on raised items.

---

## 10. Engine vs engine and comparison

- `MatchSetupScreen` lets both slots be engines. It adds a series length N, alternating starts on/off, and the playback delay.
- `MatchRunner` plays N games through the same `MatchController`. It records per-engine W/D/L, average move time and average nodes, when available.
- **UI mode:** watch each game, and the Results screen shows the running tally.
- **Headless mode:** `quarto --compare <engineA> <engineB> -n 100 [--depthA 5 --depthB 7]` runs with no window and prints a summary table. This is possible because nothing below `ui/` touches raylib.

---

## 11. Build structure

Planned CMake targets:

| Target | Type | Sources | Links |
|---|---|---|---|
| `quarto_engine` | static lib | `src/engine/*.cpp` | — |
| `quarto_core` | static lib | `src/game`, `src/players`, `src/match` | `quarto_engine` |
| `quarto` | executable | `src/main.cpp`, `src/ui/**` | `quarto_core`, `raylib` |
| `quarto_tests` | executable (optional) | `tests/*` | `quarto_core` |

- Use `target_include_directories(... PUBLIC src)` so includes read `#include "game/game_state.hpp"`.
- The current `target_include_directories(quarto PRIVATE include)` points at an `include/` directory that does not exist. Replace it when the split is made.
- Copy `assets/` next to the binary with a `POST_BUILD` step.
- The stub files are **not** in `CMakeLists.txt` yet. Add each file as it gets real code.

---

## 12. Conventions

- **Naming:** `XxxScreen`, `XxxView`, `XxxWidget`, `XxxEngine`. Files are `snake_case`, types `PascalCase`, and everything lives in `namespace quarto` (UI code in `quarto::ui`).
- **Views** hold only presentation state (hover, animation progress, scroll offset). Game state is always passed in as `const&`.
- **Globals:** none. `App` owns `Theme` and `Assets` and passes them by reference.
- **raylib types** (`Rectangle`, `Color`, `Font`) appear only in `ui/` headers.
- **One draw path per concept:** pieces through `PieceRenderer`, panels through `Panel`, text through a single theme-aware text helper.
- **Comments:** each file opens with the header comment that describes its job (already present in the stubs).

---

## 13. Implementation order

1. **`game/`**: state, rules, history, bridge, with unit tests (no UI).
2. **Static render**: `App`, `Theme`, `PieceRenderer`, `BoardView` and `PiecePoolView` drawing a fixed position.
3. **Human vs human**: `MatchController` + `Command` + `GameScreen` input. A full game can be played.
4. **Engines**: `IEngine`, `MinimaxEngine`, `EngineRegistry`, `EngineWorker`. Human vs engine without freezing.
5. **History**: `MoveListView`, undo/redo/jump, and cancelling a stale search.
6. **Analysis panel**: timing now, full stats once the engine exposes them.
7. **Engine vs engine**: setup options, playback controls, `MatchRunner`, Results screen, headless `--compare`.
8. **Polish**: animations, winning-line highlight, light theme, resize edge cases.
