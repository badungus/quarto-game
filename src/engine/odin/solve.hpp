#pragma once
#include "position.hpp"
#include "tt.hpp"
#include "types.hpp"
#include <cstdint>
#include <functional>

namespace quarto {

// Proves the game-theoretic value of a position: whether the side to move
// wins, draws or loses with perfect play from both sides.
//
// Unlike Search it does not look for the fastest win or the best score, only
// for the result, so it searches with the window (-1, 1): the first win found
// below a node cuts it off. It searches to the end of the game, with no depth
// limit, no time limit and no iterative deepening.
//
// The TT must not be shared with a Search: solver entries hold -1/0/1, search
// entries distances to a win.

constexpr Score SOLVED_WIN = 1;
constexpr Score SOLVED_DRAW = 0;
constexpr Score SOLVED_LOSS = -1;

struct SolveResult {
  Score value; // SOLVED_WIN, SOLVED_DRAW or SOLVED_LOSS for the side to move
  // A move that achieves `value`, as in Search: {cell, give}, cell -1 at the
  // opening, give -1 when the placement ends the game. When every move loses,
  // some legal move. EMPTY_MOVE only on a full board.
  Move move;
};

// Called every PROGRESS_INTERVAL nodes during a solve.
using ProgressFn = std::function<void(std::uint64_t nodes)>;

class Solver {
private:
  TranspositionTable &tt_;
  std::uint64_t nodes_ = 0;
  ProgressFn on_progress_;
  // Positions with at most this many pieces are keyed by their canonical
  // form, so equivalent positions share a TT entry; deeper ones by the
  // position as it is. The piece count decides, so a position always gets the
  // same kind of key, and the two kinds never meet.
  int canon_max_pieces_ = DEFAULT_CANON_MAX_PIECES;

  // Fail-hard alpha-beta value of `pos` for the side to move, in a position
  // where the piece in hand cannot win (the parent only gives safe pieces).
  // At the root, best_out receives the move that raised alpha, if any.
  Score negamax(Position &pos, Score alpha, Score beta,
                Move *best_out = nullptr);

public:
  static constexpr std::uint64_t PROGRESS_INTERVAL = std::uint64_t{1} << 24;
  static constexpr int DEFAULT_CANON_MAX_PIECES = 10;

  explicit Solver(TranspositionTable &tt) : tt_(tt) {}
  void set_progress(ProgressFn fn) { on_progress_ = std::move(fn); }
  // -1 turns canonical keys off. Only change it between solves, and clear the
  // TT when you do: entries stored under the other kind of key go unused.
  void set_canon_max_pieces(int pieces) { canon_max_pieces_ = pieces; }
  // `pos` is modified during the solve and restored before returning.
  SolveResult solve(Position &pos);
  // Nodes visited by the last solve.
  std::uint64_t nodes() const { return nodes_; }
};

} // namespace quarto
