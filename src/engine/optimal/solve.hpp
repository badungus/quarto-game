#pragma once
#include "position.hpp"
#include "tt.hpp"
#include "types.hpp"
#include <atomic>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace quarto {

// Proves the game-theoretic value of a position: whether the side to move
// wins, draws or loses with perfect play from both sides.
//
// It does not look for the fastest win, only for the result, so it searches
// with the window (-1, 1): the first win found below a node cuts it off. It
// searches to the end of the game, with no depth limit, no time limit and no
// iterative deepening. Every TT entry is a proven result, so the TT stays valid
// from one solve to the next.

constexpr Score SOLVED_WIN = 1;
constexpr Score SOLVED_DRAW = 0;
constexpr Score SOLVED_LOSS = -1;

inline const char *outcome(Score value) {
  if (value == SOLVED_WIN)
    return "win";
  if (value == SOLVED_LOSS)
    return "loss";
  return "draw";
}

struct SolveResult {
  Score value; // SOLVED_WIN, SOLVED_DRAW or SOLVED_LOSS for the side to move
  // A move that achieves `value`: {cell, give}, cell -1 at the
  // opening, give -1 when the placement ends the game. When every move loses,
  // some legal move. EMPTY_MOVE only on a full board.
  Move move;
};

// A canonical position: cols and occ of its Canon.
using CanonKey = std::pair<std::uint64_t, Bitboard>;
// A solved value and a move achieving it, in the canonical frame:
// from_canonical maps the move into any equivalent position.
struct BookEntry {
  Score value;
  Move move;
};
using Book = std::map<CanonKey, BookEntry>;

// The opening book written by Solver::full_solve. One position per line:
// cols and occ in hex, then value, cell and give. Lines that do not parse
// (comments start with '#') are skipped; a missing file is an empty book.
Book read_book(const std::string &path);

class Solver {
private:
  TranspositionTable &tt_;
  std::uint64_t nodes_ = 0;
  // Set from another thread to abandon the solve; null for never.
  const std::atomic<bool> *stop_ = nullptr;
  bool stopped() const {
    return stop_ && stop_->load(std::memory_order_relaxed);
  }

  std::vector<Position> create_positions_();
  void find_position(int ply, Position &pos, std::set<CanonKey> &seen,
                     std::vector<Position> &positions);
  // Value of `pos` for the side to move, `ply` turns below the empty board,
  // from the book entries of the positions SEARCH_START_PLY turns in. Adds
  // every position above those to the book, with its best move. `done` holds
  // the positions combined so far, so each is combined once.
  Score combine(int ply, Position &pos, Book &book, std::set<CanonKey> &done);

  // Positions with at most this many pieces are keyed by their canonical
  // form, so equivalent positions share a TT entry; deeper ones by the
  // position as it is. The piece count decides, so a position always gets the
  // same kind of key, and the two kinds never meet.
  static constexpr int CANON_MAX_PIECES = 10;

  // Fail-hard alpha-beta value of `pos` for the side to move, in a position
  // where the piece in hand cannot win (the parent only gives safe pieces).
  // At the root, best_out receives the move that raised alpha, if any.
  Score negamax(Position &pos, Score alpha, Score beta,
                Move *best_out = nullptr);

public:
  explicit Solver(TranspositionTable &tt) : tt_(tt) {}
  // Once *stop is true, solve() returns as soon as it can, with a meaningless
  // result, and stores nothing more in the TT, so the TT stays correct.
  void set_stop(const std::atomic<bool> *stop) { stop_ = stop; }
  // `pos` is modified during the solve and restored before returning.
  SolveResult solve(Position &pos);
  // Solves the empty board on `threads` threads sharing the TT: each solves
  // positions a few turns in, and their values are combined. Prints a line
  // per finished job.
  //
  // `book_path` is an opening book and a checkpoint at once. Each solved job
  // is appended to it as it finishes, and jobs already in it are skipped, so
  // an interrupted solve resumes where it stopped. At the end the file is
  // rewritten with every position from the empty board down to the jobs.
  // One position per line: cols and occ in hex, then value, cell and give,
  // the move in the canonical frame.
  SolveResult full_solve(unsigned threads = std::thread::hardware_concurrency(),
                         const std::string &book_path = "book.txt");
  // Nodes visited by the last solve.
  std::uint64_t nodes() const { return nodes_; }
};

} // namespace quarto
