# Paper Structure — Positional vs. Tactical Play in Quarto

**Working title:** *Is There Positional Play in Quarto? Design of a Bitboard Search
Engine and an Empirical Comparison of Evaluation Functions*

**Alternative titles**
- *Thor and Odin: Building and Testing Search Engines for the Game of Quarto*
- *Tactics All the Way Down? Evaluating Heuristic Knowledge in a Short, Fully Tactical Board Game*

**Target length:** ~20 pages (single column, 11 pt, including figures, tables and
references). The page budget for each section is given in brackets. The engine
(Sections 3–5) and the engine comparison (Sections 6–7) make up about 60 % of the
paper. The GUI only appears where it is needed to explain the user experience and
the test setup.

**Central research question**

> In Quarto, does a heuristic evaluation that encodes *positional* knowledge make
> an engine stronger than a purely *tactical* evaluation at the same search budget?
> In other words, is Quarto a game in which positional play exists, or is
> it decided by tactics that are within reach of search alone?

**Sub-questions**
1. How far can a plain alpha-beta search with a bitboard representation see in Quarto
   within a typical move time (1–3 s)?
2. How much do a transposition table and move ordering (Odin) increase search depth
   and playing strength compared with the baseline (Thor)?
3. Which evaluation features (if any) correlate with the game-theoretic value of a
   position before the search can see to the end of the game?
4. How do human players experience playing against engines with different evaluation
   functions and strength settings?

---

## Page Budget Overview

| # | Section | Pages |
|---|---|---|
| — | Title, Abstract, Keywords | 0.5 |
| 1 | Introduction | 1.5 |
| 2 | Background and Related Work | 2.0 |
| 3 | Engine Architecture | 3.5 |
| 4 | Search Algorithm (Thor baseline) | 2.5 |
| 5 | Odin: Transposition Table and Move Ordering (planned) | 2.0 |
| 6 | Evaluation Functions: Tactical vs. Positional | 2.0 |
| 7 | Experimental Setup and Engine Testing | 2.0 |
| 8 | Results | 2.0 |
| 9 | User Experience against the Engine | 1.5 |
| 10 | Discussion | 1.0 |
| 11 | Conclusion and Future Work | 0.5 |
| — | References | 1.0 |
| — | Appendix (protocol, notation, extra tables) | (outside the 20-page budget, or 1–2 extra pages) |
| | **Total** | **~20** |

---

## Abstract [0.25 pages]

About 200 words. It should cover these points:
- Context: Quarto is a two-player, perfect-information, impartial-piece game in
  which the *opponent chooses the piece you must place*.
- Contribution 1: a bitboard engine architecture (Thor) with negamax, alpha-beta,
  iterative deepening, a time-controlled search thread and a text protocol (UQI).
- Contribution 2: an extended engine (Odin) with Zobrist hashing, plus a planned
  transposition table and move ordering.
- Contribution 3: an experimental framework that compares engines that differ *only
  in their evaluation function*, to test whether positional knowledge helps.
- Contribution 4: observations on human play against the engine.
- Main finding: *[fill in after the experiments]*.

**Keywords:** Quarto, game-tree search, alpha-beta, negamax, bitboards, Zobrist
hashing, transposition table, move ordering, evaluation function, engine testing,
Elo, human–computer play.

---

## 1 Introduction [1.5 pages]

### 1.1 Motivation
- Most work on game engines comes from chess, where "positional play" (long-term,
  non-forcing advantages) is a well-known idea. It is an open question whether such
  a concept exists in very short games with a high branching factor, such as Quarto.
- What makes Quarto special: a move is a **pair** (place the given piece, then give
  a piece to the opponent). The "give" half of the move is a defensive choice that
  chess has no equivalent for.
- The game is short (at most 16 placements), but the early game has a very large
  branching factor. Search cannot see to the end in the opening, but it can easily
  do so in the endgame. This makes the *middle game* the interesting place where
  evaluation knowledge could matter.

### 1.2 Problem Statement
- Is positional evaluation useful in Quarto, or does any gain come only from
  detecting threats (tactics)?

### 1.3 Contributions
1. A detailed description of a modular, bitboard-based Quarto engine (Thor).
2. A second engine generation (Odin) built for transposition tables and move
   ordering. Hashing is implemented; the table and the ordering are designed but
   not yet implemented.
3. A text protocol (UQI, modelled on UCI) that separates engine and GUI, so that
   any number of engines can be plugged in and tested against each other.
4. A test method for evaluation functions in which everything else is held constant.
5. A qualitative and quantitative look at the user experience of playing against
   the engine.

### 1.4 Structure of the Paper
- A short overview of each section.

---

## 2 Background and Related Work [2.0 pages]

### 2.1 The Game of Quarto
- Origin: designed by Blaise Müller (published by Gigamic, 1991).
- Rules: 4×4 board, 16 pieces, 4 binary properties. You win by completing a row,
  column or diagonal of four pieces that share at least one property.
- **Figure 1:** board with the cell indexes 0–15 and the algebraic notation a1–d4,
  next to the 16 pieces with their four property bits.
- Formal model: a two-player, zero-sum, deterministic game with perfect information.
  A ply is a (place, give) pair. The first ply of the game is give-only.

### 2.2 Complexity of Quarto
- Branching factor at ply *k* (with *k* pieces placed): (16 − k) cells × (15 − k)
  pieces. It is 16 for the first (give-only) move and 16·15 = 240 for the second.
- Upper bound on the number of move sequences: 16! · 16! ≈ 4.4 · 10²⁶.
- State-space estimate *[compute: Σₖ placements × piece assignments × piece in hand]*.
- Symmetries: the board has geometric symmetries (rotations, reflections and the
  less obvious line-preserving permutations), and the pieces have symmetries too
  (permuting the 4 properties and inverting any of them). *[Verify the exact counts
  from the literature: 32 board symmetries are commonly cited.]*
- Known result: Quarto has been solved (Luc Goossens, 1998). With perfect play it
  is a draw. *[Verify the citation and the exact result.]*

### 2.3 Game-Tree Search
- Minimax and negamax (von Neumann; Knuth & Moore 1975).
- Alpha-beta pruning: in the best case (perfect move ordering) it searches about
  b^(d/2) nodes, in the worst case b^d.
- Iterative deepening and time control.
- Transposition tables and Zobrist hashing (Zobrist 1970; Marsland & Campbell).
- Move ordering heuristics: hash move, killer moves (Akl & Newborn 1977), the
  history heuristic (Schaeffer 1989).

### 2.4 Evaluation Functions and the Tactical/Positional Distinction
- In chess, material, mobility and king safety act as positional proxies. In
  tactical games such as Connect Four, threats and parity (Allis 1988) matter more.
- How we define the terms in this paper:
  - **Tactical knowledge:** facts about forced sequences within the search horizon
    or just beyond it (immediate wins, threats, "poisoned" pieces).
  - **Positional knowledge:** static features that predict the outcome without a
    forcing sequence (line potential, safe-piece parity, structure).

### 2.5 Engine Protocols and Engine Testing
- The UCI protocol (Huber & Meyer-Kahlen, 2000) as the model for UQI.
- Testing methods: fixed-depth and fixed-time matches, Elo / BayesElo, SPRT
  (as used in Stockfish's Fishtest), balanced opening sets.

### 2.6 Prior Work on Quarto AI
- Quarto as a General Game Playing benchmark; MCTS approaches; student and hobby
  engines. *[Literature search needed. Note any gap: to our knowledge, no
  systematic study of evaluation functions in Quarto exists.]*

---

## 3 Engine Architecture [3.5 pages]

*Main focus of the paper. Describe the code base at the level of data structures
and responsibilities. Use short code excerpts only where they make a point clearer.*

### 3.1 Design Goals
- Speed (bitboards, no heap allocation in the inner search loop).
- The engine is a **separate process**. It communicates over stdin/stdout, so
  engines can be swapped, crashes stay isolated and the GUI stays responsive.
- **Engine generations as separate executables** (`thor`, `odin`, …) that share
  the same interface. A new engine is one directory plus one line in the engine list
  (`ENGINES` in `match_config.hpp`).
- Reproducible behaviour (`constexpr` tables, a fixed Zobrist seed) together with
  controlled randomness (random tie-breaking between moves of equal value).

### 3.2 Module Overview
- **Figure 2:** component diagram:
  `main → uqi_loop → Engine → Search → Position / wincheck / tables`.
- **Table 1:** module responsibilities.

| Module | Responsibility |
|---|---|
| `types.hpp` | `Bitboard` (uint16), `Key` (uint64), `Piece`, `Cell`, `Score`, `Move{cell, give}`, `Undo`; constants `WIN = 10000`, `INF = 32000` |
| `tables.hpp` | `constexpr` line masks, per-cell line lookup, Zobrist keys |
| `position.*` | Board state, make/unmake of placing and giving |
| `wincheck.*` | Line-win detection, static evaluation |
| `search.*` | Iterative deepening, root search, negamax with alpha-beta |
| `engine.*` | Search thread, limits (depth, movetime, infinite), reset after `go` |
| `uqi.*` | Protocol parser, position parser, move formatting |
| `tt.*`, `ordering.*` | (Odin) transposition table and move ordering, *planned* |

### 3.3 Board Representation
- A 16-bit occupancy bitboard `occ_`.
- **Four property bitboards** `prop_[0..3]`: bit *c* of `prop_[i]` is set when the
  piece on cell *c* has property *i*. A piece is never stored as an object. It
  exists only as one bit in each of the four property planes.
- The available-pieces bitboard `avail_` (pieces neither placed nor in hand), and
  `hand_` (the piece the side to move must place).
- Why this layout works well: a win check comes down to a few AND/compare
  operations per line. It needs no loop over pieces.
- **Figure 3:** example position split into its `occ` and `prop[0..3]` planes.

### 3.4 Line Tables and Win Detection
- `LINES`: 10 masks (4 rows, 4 columns, 2 diagonals).
- `CELL_LINES`: a table built at compile time (`constexpr`) that lists the 2–3 lines
  through each cell, so a win check after a placement only looks at those lines.
- `line_is_win`: a line is a win if it is full **and** for some property *p*
  `prop[p] & line == line` (all have the property) or `~prop[p] & line & occ == line`
  (none has it).
- Cost analysis: at most 3 lines × 4 properties × 2 comparisons per placement.

### 3.5 Make/Unmake and the Two-Phase Move
- A Quarto ply is split into `place_piece(cell)` and `give_piece(piece)`, each with
  its own undo (`undo_place_piece`, `undo_give_piece`).
- A fixed-size history `std::array<Undo, 17>`: 16 placements plus the opening
  give. There is no allocation during the search.
- Special case: the opening give. When nothing has been placed yet, undoing the give
  returns the hand to "empty".
- Discuss: why make/unmake instead of copy-make (the position is small, about 30
  bytes, so both are possible. Measure the difference *[optional micro-benchmark]*).

### 3.6 Zobrist Hashing (Odin)
- Keys: `ZOBRIST[cell][piece]` (256 keys) and `HELD[piece]` (16 keys for the piece
  in hand).
- Generated at compile time with splitmix64 and a fixed seed, so every run uses the
  same table.
- Updated incrementally in `place_piece`, `give_piece` and both undo functions.
  `load()` computes the key from scratch.
- Why the piece in hand must be part of the key: two positions with the same board
  but a different piece in hand are different game states.
- Collision probability for 64-bit keys at the expected table sizes *[short
  calculation]*.

### 3.7 Concurrency and Search Control
- `Engine::go` starts a `std::jthread` that searches its **own copy** of the
  position, so the protocol thread can accept the next commands at once.
- `std::atomic<bool> stop_`: set by `stop`, by `quit`, or when the time runs out.
  The clock is checked every 4096 nodes (`nodes_ & 4095`).
- Depth limit: `min(depth, number of empty cells)`.
- After each `go` the engine resets to its defaults, so every request is
  self-contained and the engine is stateless between requests.

### 3.8 The UQI Protocol (Universal Quarto Interface)
- Commands: `uqi` → `uqiok`, `isready` → `readyok`, `seed N`,
  `go [depth N] [movetime MS] [infinite] position <FEN-like>`, `stop`, `quit`.
- Output: `info seed N` (once at the first search, and again after each new
  seed, so every game log can be replayed), then
  `info depth D score S [mate N] nodes K move M` for each completed iteration,
  then `bestmove M`. `mate N` gives the distance to a proven result in turns of the
  side to move (N > 0 win, N < 0 loss). Lines are written through `std::osyncstream` so output from the two
  threads never mixes.
- **Position notation:** four ranks separated by `/`. Each piece is 4 letters
  (`S/s C/c H/h F/f`, upper case = property bit set), `.` marks an empty cell, and
  `|` is followed by the piece in hand or `-`. The input is checked with a regular
  expression.
  Example: `ScHfschf../ScHF..scHF/..../...schF|-`.
- **Move notation:** `<cell>|<piece>`, e.g. `c3|sCHf`. `-` means no placement
  (opening move) or no give (game over), and `#` marks a winning placement.
- Cell mapping: cell 12 = a1 (rows run top to bottom, ranks bottom to top).
- **Listing 1:** an example UQI session.

### 3.9 GUI Integration (only the parts needed later)
- The GUI starts the engine executable as a child process (tiny-process-library)
  and reads its output line by line on a reader thread.
- **Stale result handling:** after a `stop` (undo, new game) the engine still sends
  a `bestmove` for the old position. The GUI counts these (`stale_bestmoves_`)
  and throws them away.
- `info` lines are stored for each half-move (17 slots), so the evaluation display
  stays correct after undo and redo.
- Player seats: human or engine for each side. This makes **engine-vs-engine games
  inside the GUI** possible, which is used for qualitative testing in Section 7.

---

## 4 Search Algorithm — Thor (Baseline) [2.5 pages]

### 4.1 Negamax with Alpha-Beta
- **Algorithm 1 (pseudo-code):** a nested loop over empty cells × available pieces.
  The place→win check and the full-board draw check come *before* the give loop.
- Fail-hard bounds, recursion with `(-beta, -alpha)`.
- A cutoff in the inner (give) loop ends the search of the whole node.

### 4.2 Root Search with All Best Moves
- The root searches every child with a **full window** `(-INF, INF)` on purpose.
  If the window were narrowed, a fail-high could not be told apart from an exact
  tie.
- It collects *all* moves that tie for the best score and picks one **uniformly
  at random**.
- Consequences: variety in play (important for the user experience and for varied
  test games), but less pruning at the root. Quantify how many extra nodes this
  costs *[measure]*.
- Special case: the opening position (no piece in hand), where the root only loops
  over gives.

### 4.3 Iterative Deepening and Time Control
- Depth 1 … D. A depth that is interrupted is thrown away, and the result always
  comes from the deepest *completed* depth.
- Early stop when a forced win or loss is proven (`|score| ≥ WIN − i`).
- One `info` line per depth, which drives the GUI's evaluation display and log.

### 4.4 Scoring Conventions
- Win scores depend on depth: `WIN + depth_remaining`, so faster wins score higher
  and slower losses are preferred.
- Draw = 0. Non-terminal leaves get the static evaluation (Section 6).

### 4.5 Thor's Static Evaluation (the "Tactical Baseline")
- `eval(pos)`: if *any* empty cell would complete a winning line with the piece in
  hand → `WIN`, otherwise `0`.
- This is in effect a one-ply look-ahead for an immediate win at the horizon.
- It has **no positional knowledge at all**. It is the null hypothesis of this
  paper.

### 4.6 Baseline Performance
- **Table 2:** nodes, time and depth reached for each game phase (number of empty
  cells), measured on a fixed set of test positions.
- **Figure 4:** depth reached in 1 s / 3 s as a function of empty cells. We expect
  shallow depth in the opening and a full solve once fewer than about *N* cells are
  empty *[measure N]*.
- Effective branching factor before and after alpha-beta.

### 4.7 Known Limitations of Thor
- No transposition table: transpositions are very common in Quarto (the same pieces
  on the same cells in a different order), and each one is searched again.
- No move ordering: cells and pieces are visited in bit order (`countr_zero`), so
  alpha-beta is far from its best case.
- Search effort is spent again at every iteration of iterative deepening.

---

## 5 Odin — Transposition Table and Move Ordering (Planned) [2.0 pages]

*State clearly that this is **design and expected effect**. Odin currently has
Zobrist hashing (Section 3.6). The files `tt.cpp`/`tt.hpp` and
`ordering.cpp`/`ordering.hpp` exist but are empty. Results for Odin are either
reported later or presented as projections.*

### 5.1 Transposition Table Design
- Entry layout: key (or its upper bits), score, depth, bound type
  (EXACT / LOWER / UPPER), best move `{cell, give}`. Size *[e.g. 16 bytes]*.
- Indexing: `key & (size − 1)` with a power-of-two table size.
- Replacement scheme: depth-preferred vs. always-replace vs. two-tier buckets.
- **Mate-score adjustment:** scores of the form `WIN + depth` must be converted to
  distance-from-node when stored and back when read.
- Interaction with the root's "collect all ties" design: TT cutoffs are not allowed
  at the root, or the result would no longer contain every tied move.
- Optional: canonicalise positions under board and piece symmetries before hashing
  (Section 2.2). This could save a large factor, but costs time to compute the
  canonical form *[discuss as an extension]*.
- Thread safety: one search thread for each engine, so no locking is needed.

### 5.2 Move Ordering Design
- Order of priority:
  1. **TT / PV move** from the previous iteration.
  2. **Immediate wins** with the piece in hand (already caught by the place→win
     check, but they should come first).
  3. **Safe gives first:** pieces with which the opponent *cannot* win at once.
     Unsafe gives always lose and can be sorted last or cut early (a tactical
     pruning rule).
  4. **Killer moves** for each depth, for both the cell half and the give half.
  5. **History heuristic** over `(cell, piece)` pairs.
- Quarto-specific point: ordering has two layers, cell order and give order. Each
  layer can use its own heuristic.

### 5.3 Expected Effect and How to Measure It
- Metrics: nodes to depth D, time to solve, effective branching factor, and the
  share of cutoffs that happen on the first move.
- **Table 3 (planned):** ablation Thor vs. Odin+TT vs. Odin+ordering vs. Odin+both,
  on the same position set.
- Strength gain in Elo at fixed time (more depth in the same time).

### 5.4 Implementation Status
- A clear list of what exists (hash keys, incremental updates, build targets)
  and what is still to do (TT probe/store in `negamax`, ordering functions,
  ablation runs).

---

## 6 Evaluation Functions: Tactical vs. Positional [2.0 pages]

*Core of the research question. Each evaluation function becomes its own engine
(or a compile-time or UQI option) with the **same search**, so that any difference
in strength comes from the evaluation only.*

### 6.1 Requirements for an Evaluation in Quarto
- It is evaluated from the side to move, who **holds** a piece they did not choose.
- The scale must stay well below `WIN` (e.g. |eval| < 1000) so it is never mistaken
  for a proven result.
- It must be cheap, because it runs at every leaf.

### 6.2 Candidate Evaluation Functions

| ID | Name | Type | Idea |
|---|---|---|---|
| E0 | Null | control | Always 0. A pure search baseline. |
| E1 | Immediate-win (Thor) | tactical | `WIN` if the piece in hand wins somewhere, else 0 |
| E2 | Threat count | tactical | Number of lines with 3 pieces that share a property and have one empty cell, plus which remaining pieces would complete them |
| E3 | Safe-piece parity | tactical/positional | Number of pieces that can be given without allowing an immediate win, and whether that number is even or odd (like Connect Four's zugzwang/parity) |
| E4 | Line potential | positional | For each line with 2 pieces: the number of properties they share, weighted |
| E5 | Structural / central control | positional | Weight of cells on 3 lines (diagonals) vs. 2 lines, and the variety of properties on the board |
| E6 | Combined (weighted) | mixed | A linear combination of E2–E5. Weights tuned by self-play or regression against solved positions |
| E7 | Random noise | control | Small random value. Tests whether "any" evaluation beats E0 |

- Give a formal definition and a cost estimate for each one (bitboard formulas,
  e.g. a threat test using `popcount(line & occ) == 3` together with the property
  planes).

### 6.3 Hypotheses
- **H1 (tactical dominance):** At equal search budget, E2/E3 are significantly
  stronger than E0/E1, but E4/E5 add *no* significant strength on top of E2/E3.
- **H2 (phase dependence):** Any positional effect shows up only in the early
  middle game (about 9–12 empty cells), where search cannot yet reach the end of
  the game.
- **H3 (parity):** Safe-piece parity (E3) is the strongest single feature, which
  would match the role of parity in Connect Four.
- **H4 (depth substitutes knowledge):** The Elo gap between evaluations shrinks as
  search depth grows and disappears at full-solve depth.

### 6.4 Measuring "Positional Value" Directly
- Ground truth: positions with ≤ *N* empty cells are solved exactly by the engine
  itself (full-depth search).
- For each evaluation: the correlation (or AUC) between the static score and the
  true result (win/draw/loss), for each game phase.
- **Figure 5:** predictive accuracy of each evaluation vs. number of empty cells.
  If positional play exists, E4/E5 should predict better than chance *after*
  tactical features are controlled for.

---

## 7 Experimental Setup and Engine Testing [2.0 pages]

### 7.1 Engines Under Test
- **Thor** (baseline: E1, no TT, no ordering).
- **Odin** (Thor + Zobrist; TT and ordering once implemented).
- **Evaluation variants** E0–E7, built on one fixed search core (preferably Odin
  once it is finished, otherwise Thor).
- **Future engines** ("and more to come"): describe the plug-in path. The same UQI
  protocol, one entry in `ENGINES`, and the engine is available to the GUI and to
  the test tool. Possible later engines: MCTS, a solver with an endgame database, a
  learned evaluation.

### 7.2 Test Harness
- A headless match runner *[to be built]* that talks UQI to two engine processes,
  alternates the starting player, and records PGN-like game logs (positions, moves,
  `info` lines).
- The GUI's engine-vs-engine mode is used for checking games by eye and for finding
  bugs, but not for statistics.

### 7.3 Controlling for Confounders
- **Openings:** a fixed set of *K* start positions (e.g. random positions with 2–4
  placed pieces that are balanced by full search). Each is played twice with the
  sides swapped.
- **Built-in randomness:** the random choice among tied best moves already makes
  games differ. Report the seeds.
- **Search budget:** two regimes:
  - *fixed depth* (isolates the evaluation itself),
  - *fixed time* (includes the speed cost of the evaluation).
- Hardware, compiler and flags (`-O2`/Release, GCC version) are fixed and reported.

### 7.4 Metrics
- Win/draw/loss, score %, Elo difference with 95 % confidence interval
  (BayesElo or Ordo).
- SPRT for pairwise decisions (H0: ΔElo ≤ 0, H1: ΔElo ≥ 10). α = β = 0.05.
- Draw rate. The game is a theoretical draw, so we expect a high draw rate among
  strong engines.
- Average game length, and the ply at which the result is proven (first `|score| ≥
  WIN − d`).
- Search statistics: nodes/s, depth reached, time to the first proven result.

### 7.5 Test Plan

| Experiment | Compares | Budget | Purpose |
|---|---|---|---|
| X1 | Thor vs. Odin(TT), Odin(ord), Odin(TT+ord) | fixed time | Value of the search improvements |
| X2 | E0…E7 round robin | fixed depth 2, 3, 4 | Pure evaluation quality |
| X3 | E0…E7 round robin | fixed time 1 s, 3 s | Practical strength |
| X4 | Static evaluation vs. solved positions | — | Predictive accuracy for each phase (H2) |
| X5 | Best tactical vs. best combined | increasing depth | Does depth replace knowledge? (H4) |

### 7.6 Correctness Testing
- Perft-like node counts for fixed positions (they must be the same for Thor and
  Odin when the TT is off).
- Win detection unit tests (every line, every property, both property values).
- UQI round-trip tests (parse → format → parse).
- Consistency check: Odin with TT must return the same score as Thor at equal depth.

---

## 8 Results [2.0 pages]

*Placeholders. Fill in after the experiments. Report only measured values.*

### 8.1 Search Performance (X1)
- **Table 4:** nodes and time to depth for each engine and each phase.
- **Figure 6:** depth reached against time, Thor vs. Odin variants.

### 8.2 Evaluation Tournament (X2, X3)
- **Table 5:** cross-table with Elo ± CI, W/D/L and draw rate.
- **Figure 7:** Elo by evaluation function, grouped as tactical / positional /
  control.

### 8.3 Predictive Accuracy (X4)
- **Figure 5** (from 6.4) with the measured values.
- Partial correlation of the positional features after the tactical features are
  controlled for.

### 8.4 Depth vs. Knowledge (X5)
- **Figure 8:** Elo gap against search depth.

### 8.5 Hypotheses Summary
- **Table 6:** H1–H4, each marked supported / not supported, with the evidence.

---

## 9 User Experience against the Engine [1.5 pages]

### 9.1 Interaction Design (short)
- Seat selection (human/engine for each side), a piece pool for giving, and
  clicking a cell for placing.
- Match controls: pause/resume, undo/redo (half-move steps that also cancel a
  running search), stop search ("move now"), restart.
- Engine transparency: an **evaluation bar** (the score mapped to −WIN…+WIN) and a
  per-depth info log (depth, score, move) for each half-move. Both stay correct
  through undo and redo.
- Winning moves are marked with `#`.
- **Figure 9:** screenshot of the game screen with the evaluation bar and info log.

### 9.2 Observations and Issues Specific to Quarto
- **The binary evaluation problem:** with E1 the evaluation bar mostly shows 0 or
  ±WIN. It tells the player little until a forced result is found, and then it
  jumps abruptly. Positional evaluations (E4–E6) could give more readable feedback,
  even if they add no strength. This is a UX argument separate from strength.
- **"Engine knows it has won" effect:** once a forced win is in the search horizon,
  the engine stops early and plays at once. Players can see the proven result
  before it happens on the board.
- **Variety:** random tie-breaking means the engine does not repeat the same game,
  so it cannot be beaten by memorising a sequence.
- **Difficulty control:** depth and movetime as strength levels. Compare this with
  using a weaker evaluation as a difficulty setting.

### 9.3 User Study (planned, small scale)
- Participants: *n* ≈ 10–20, a mix of new and experienced Quarto players.
- Tasks: games against Thor at 2–3 strength levels and against one positional
  variant, with the evaluation bar shown or hidden.
- Measures: win rate, game length, and a questionnaire (perceived difficulty,
  perceived "human-likeness", usefulness of the evaluation bar; SUS for usability).
- Questions: Can players tell a tactical engine from a positional one? Does a more
  graded evaluation bar help players learn?
- *[Report results, or present this as a design for future work if there is not
  enough time.]*

---

## 10 Discussion [1.0 page]

- Interpretation: what the results say about whether "positional play" exists in
  Quarto.
  - If H1 holds: Quarto is tactical. Knowledge beyond threat and parity detection
    is not worth its cost, and depth (via TT and ordering) is the main way to gain
    strength.
  - If not: name the positional features that matter and when they matter.
- Comparison with chess (strongly positional) and Connect Four (parity and threats).
- Engineering lessons: the bitboard property planes, process isolation and UQI, and
  the root search that keeps all ties.
- Threats to validity: small opening set, a single hardware setup, an evaluation
  scale that is not calibrated across variants, Odin still being incomplete, and
  the small sample in the user study.

---

## 11 Conclusion and Future Work [0.5 pages]

- A summary of the contributions and the answer to the research question.
- Future work:
  - Finish Odin's transposition table and move ordering (Section 5).
  - Symmetry-reduced hashing.
  - Further engines: MCTS, an endgame database, an evaluation learned from
    solved positions.
  - Parallel search.
  - Expand the user study, and add an adaptive difficulty setting.

---

## References [1.0 page]

*Collect and verify. Use one citation style throughout (e.g. IEEE or ACM).*
- Knuth, D. E., & Moore, R. W. (1975). An analysis of alpha-beta pruning.
- Zobrist, A. L. (1970). A new hashing method with application for game playing.
- Schaeffer, J. (1989). The history heuristic and alpha-beta search enhancements in
  practice.
- Akl, S. G., & Newborn, M. M. (1977). The principal continuation and the killer
  heuristic.
- Marsland, T. A. (1986). A review of game-tree pruning.
- Allis, L. V. (1988). A knowledge-based approach of Connect-Four.
- Allis, L. V. (1994). Searching for solutions in games and artificial intelligence.
- Goossens, L. (1998). Quarto solution *[find the primary source]*.
- Huber, R., & Meyer-Kahlen, S. (2000). Universal Chess Interface specification.
- Elo, A. (1978). The rating of chessplayers, past and present.
- Wald, A. (1945). Sequential tests of statistical hypotheses (SPRT).
- Browne, C. et al. (2012). A survey of Monte Carlo tree search methods.
- Brooke, J. (1996). SUS: a "quick and dirty" usability scale.
- *[Quarto AI / MCTS / GGP papers from the literature search]*

---

## Appendix (optional, outside the page budget)

- **A.** Full UQI specification (grammar of commands, position and move notation).
- **B.** Bitboard formulas for every evaluation feature.
- **C.** The opening position set.
- **D.** Full cross-tables and per-opening results.
- **E.** User study questionnaire.

---

## Checklist: Facts Taken from the Code Base

Use these when writing, so the paper matches the implementation:

- Bitboard = `uint16_t`, Key = `uint64_t`, Score = `int16_t`, `WIN = 10000`,
  `INF = 32000`.
- Cell index = `row * 4 + col`. Piece = 4 property bits. `-1` = none.
- Undo history: `std::array<Undo, 17>`.
- Time check every 4096 nodes. The depth is capped at the number of empty cells.
- Engine defaults: depth 16, movetime unlimited (`INT_MAX`). The GUI's engine seats
  use depth 16 with 3000 ms (`match_config.hpp`).
- Thor and Odin currently have **the same search and evaluation**. Odin only adds
  Zobrist hashing (`ZOBRIST[16][16]`, `HELD[16]`, splitmix64, seed
  `0x0DDC0FFEEBADF00D`). `tt.*` and `ordering.*` are empty.
- The engine runs in a separate process. The GUI uses tiny-process-library and
  handles stale `bestmove` replies after `stop`.
- Commands in the GUI: SelectCell, SelectGive, Undo, Redo, JumpTo, NewGame, Pause,
  Resume, Step, Stop (not all of them have UI controls yet).
