#pragma once

// The search features of this engine, each switched on by a compile definition
// (CMakeLists.txt sets them per preset). With all of them off the engine is
// baldur. Code tests them with `if constexpr`, so every branch is compiled in
// every preset, but a switched-off feature costs nothing at runtime.

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

// A win scores WIN minus the pieces on the board after it (win_score), which
// depends only on the position. Off: WIN plus the depth left, which depends on
// the path the search took to the position.
constexpr bool PIECE_COUNT_SCORE = Q_PIECE_COUNT_SCORE;

// Every node first checks whether the piece in hand wins on any empty cell
// (hand_wins), before the horizon and before searching any placement, so a
// losing give from the parent costs a few win checks instead of whole
// subtrees. Off: each placement is checked for a win in the placement loop,
// after the subtrees of the cells before it.
constexpr bool WIN_CHECK_AT_ENTRY = Q_WIN_CHECK_AT_ENTRY;

// The leaf evaluation scores a position as lost if every placement + give lets
// the opponent win at once, else 0. Off: a win if the piece in hand can win,
// else 0. It assumes the piece in hand cannot win, which WIN_CHECK_AT_ENTRY
// has checked by then, and scores the loss on PIECE_COUNT_SCORE's scale.
constexpr bool THREAT_EVAL = Q_THREAT_EVAL;
static_assert(!THREAT_EVAL || (PIECE_COUNT_SCORE && WIN_CHECK_AT_ENTRY),
              "THREAT_EVAL needs PIECE_COUNT_SCORE and WIN_CHECK_AT_ENTRY");

// The root searches each move with the window (best - 1, INF), best being the
// best score so far: a worse move fails low and is cut off early, while a tie
// or a better move still gets its exact score. Off: the full window
// (-INF, INF) for every root move.
constexpr bool NARROW_ROOT_WINDOW = Q_NARROW_ROOT_WINDOW;

// THREAT_EVAL finds the pieces that would let the opponent win at once with a
// table lookup per property (winning_pieces, LINE_COMPLETES /
// SQUARE_COMPLETES) instead of trying every give with hand_wins. Same result,
// faster leaves.
constexpr bool COMPLETES_TABLE = Q_COMPLETES_TABLE;
static_assert(!COMPLETES_TABLE || THREAT_EVAL,
              "COMPLETES_TABLE needs THREAT_EVAL");

// Only safe gives are searched, in negamax and at the root after a placement:
// pieces that cannot complete a line (available & ~winning_pieces). A piece
// that can loses at once, so it is never better than a safe one, and if no
// placement leaves a safe give the position is an exact loss. The piece in
// hand then never wins, so WIN_CHECK_AT_ENTRY's check is skipped, and a node
// with 15 pieces on the board is a draw. Off: every available piece is
// searched.
constexpr bool SAFE_GIVES = Q_SAFE_GIVES;
static_assert(!SAFE_GIVES || COMPLETES_TABLE,
              "SAFE_GIVES needs COMPLETES_TABLE");

// A transposition table (tt.cpp, 2048 MB, cleared at every go): negamax looks
// a position up before searching it and cuts off on a stored score that is
// deep enough, or proven, and fits the window; every searched position is
// stored with its score, bound, depth and best move. Keys are the exact
// position. Scores must not depend on the path, hence PIECE_COUNT_SCORE. Off:
// no table (the engine allocates one entry).
constexpr bool TT = Q_TT;
static_assert(!TT || PIECE_COUNT_SCORE, "TT needs PIECE_COUNT_SCORE");

// The best move the TT stored for a position is searched first, before the
// placement loop, if it is legal and its give is safe here; the loop then
// skips it. A good first move makes a cutoff likely before the other moves
// are searched. Off: the stored move is not used.
constexpr bool TT_MOVE = Q_TT_MOVE;
static_assert(!TT_MOVE || (TT && SAFE_GIVES),
              "TT_MOVE needs TT and SAFE_GIVES");

// Positions with at most CANON_MAX_PIECES pieces are keyed in the TT by their
// canonical form (canonical.cpp): all positions equivalent under the 32 board
// symmetries x 384 piece relabelings (8 board symmetries when squares win)
// share one entry, and its move is stored in the canonical frame. Off: every
// position is keyed as it is.
constexpr bool CANONICAL = Q_CANONICAL;
static_assert(!CANONICAL || TT, "CANONICAL needs TT");

// The root searches the previous iteration's pick first: its cell first, and
// its give first on that cell. With NARROW_ROOT_WINDOW a good first move lets
// the worse ones after it fail low sooner. Off: cells and pieces in index
// order.
constexpr bool ROOT_BEST_FIRST = Q_ROOT_BEST_FIRST;

// negamax searches the cells by a threat prior (compute_prior in
// ordering.cpp), lowest first: +2 per property on which the cell blocks a line
// of three with the value the hand lacks, -1 per line of three with the
// hand's value the placement would newly create. Off: cells in index order.
constexpr bool THREAT_ORDERING = Q_THREAT_ORDERING;
static_assert(!THREAT_ORDERING || COMPLETES_TABLE,
              "THREAT_ORDERING needs COMPLETES_TABLE");

} // namespace quarto
