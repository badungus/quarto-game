#pragma once

// Every possible feature the engine can use. Set at compile time so theres no
// runtime cost
#ifndef Q_PIECE_COUNT_SCORE
#define Q_PIECE_COUNT_SCORE 0
#endif
#ifndef Q_WIN_CHECK_AT_ENTRY
#define Q_WIN_CHECK_AT_ENTRY 0
#endif
#ifndef Q_THREAT_EVAL
#define Q_THREAT_EVAL 0
#endif
#ifndef Q_NARROW_ROOT_WINDOW
#define Q_NARROW_ROOT_WINDOW 0
#endif
#ifndef Q_COMPLETES_TABLE
#define Q_COMPLETES_TABLE 0
#endif
#ifndef Q_SAFE_GIVES
#define Q_SAFE_GIVES 0
#endif
#ifndef Q_TT
#define Q_TT 0
#endif
#ifndef Q_TT_MOVE
#define Q_TT_MOVE 0
#endif
#ifndef Q_CANONICAL
#define Q_CANONICAL 0
#endif
#ifndef Q_ROOT_BEST_FIRST
#define Q_ROOT_BEST_FIRST 0
#endif
#ifndef Q_THREAT_ORDERING
#define Q_THREAT_ORDERING 0
#endif

namespace quarto {

// a win scores WIN - pieces on the board
constexpr bool PIECE_COUNT_SCORE = Q_PIECE_COUNT_SCORE;

// Every node first checks if the piece in hand wins on any empty cell, so the
// subtrees can get pruned more effectively
constexpr bool WIN_CHECK_AT_ENTRY = Q_WIN_CHECK_AT_ENTRY;

// The leaf evaluation scores a position as lost if every placement + give lets
// the opponent win at once, else 0
constexpr bool THREAT_EVAL = Q_THREAT_EVAL;
static_assert(!THREAT_EVAL || (PIECE_COUNT_SCORE && WIN_CHECK_AT_ENTRY),
              "THREAT_EVAL needs PIECE_COUNT_SCORE and WIN_CHECK_AT_ENTRY");

// The root searches each move with the window (best - 1, INF), instead of
// (-INF, INF)
constexpr bool NARROW_ROOT_WINDOW = Q_NARROW_ROOT_WINDOW;

// Uses a table lookup for each property and position to find pieces that let
// the opponent win at once, instead of computing it every node from scratch
constexpr bool COMPLETES_TABLE = Q_COMPLETES_TABLE;
static_assert(!COMPLETES_TABLE || THREAT_EVAL,
              "COMPLETES_TABLE needs THREAT_EVAL");

// Only safe gives are searched, in negamax and at the root after a placement
constexpr bool SAFE_GIVES = Q_SAFE_GIVES;
static_assert(!SAFE_GIVES || COMPLETES_TABLE,
              "SAFE_GIVES needs COMPLETES_TABLE");

// Transposition Table, negamax looks a position up before searching
constexpr bool TT = Q_TT;
static_assert(!TT || PIECE_COUNT_SCORE, "TT needs PIECE_COUNT_SCORE");

// Searching the TT move firste
constexpr bool TT_MOVE = Q_TT_MOVE;
static_assert(!TT_MOVE || (TT && SAFE_GIVES),
              "TT_MOVE needs TT and SAFE_GIVES");

// Storing equivalent positions using the same canonical form
constexpr bool CANONICAL = Q_CANONICAL;
static_assert(!CANONICAL || TT, "CANONICAL needs TT");

// The root searches the previous iteration's pick first
constexpr bool ROOT_BEST_FIRST = Q_ROOT_BEST_FIRST;

// negamax searches the cells by a threat prior
constexpr bool THREAT_ORDERING = Q_THREAT_ORDERING;
static_assert(!THREAT_ORDERING || COMPLETES_TABLE,
              "THREAT_ORDERING needs COMPLETES_TABLE");

} // namespace quarto
