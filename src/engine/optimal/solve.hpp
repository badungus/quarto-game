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
// wins, draws or loses with perfect play from both sides, and how soon.
//
// A win scores WIN - n, with n the pieces on the board after the winning
// placement, so the sooner the win, the higher the score; a loss scores the
// negation, a draw 0. The score depends only on the piece count, not on the
// path, so equivalent positions share it and it can go in the TT.
//
// A solve searches twice. First with the window (-1, 1), for the result only:
// every win scores at least 1, so the first win found below a node cuts it
// off, as fast as a search that knows no distances. A draw is done then. A
// win or loss is searched again with a window that holds only wins (or only
// losses), for the exact score: moves that do not win fail low at once, and
// the first search left the TT full of winning moves to try first. Both
// searches go to the end of the game, with no depth limit, no time limit and
// no iterative deepening. Every TT entry is a bound on a proven score, so the
// TT stays valid from one solve to the next.

constexpr Score SOLVED_DRAW = 0;
// Every win scores at least this, as a full board holds 16 pieces, and every
// loss at most its negation.
constexpr Score PROVEN = WIN - 16;
// The score of a win by the placement that makes `pieces` pieces on the board.
constexpr Score win_score(int pieces) { return Score(WIN - pieces); }

inline const char *outcome(Score value) {
  if (value > 0)
    return "win";
  if (value < 0)
    return "loss";
  return "draw";
}

// The value as UQI's mate: n when the side to move wins with its nth
// placement from `pos`, -n when the opponent wins with its nth, 0 for a draw
// or a value without a distance (a win or loss from a book written before
// solves had distances, stored as 1 or -1).
int mate_distance(Score value, const Position &pos);

struct SolveResult {
  Score value; // the score for the side to move, as above
  // A move that achieves `value`: {cell, give}, cell -1 at the
  // opening, give -1 when the placement ends the game. When every move loses,
  // one that loses as late as possible. EMPTY_MOVE only on a full board.
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
// Books from before solves had distances store wins and losses as 1 and -1.
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
  // same kind of key, and the two kinds never meet. -1 turns canonical keys
  // off, which only the check program does.
  int canon_max_pieces_ = 10;

  // Fail-hard alpha-beta value of `pos` for the side to move, in a position
  // where the piece in hand cannot win (the parent only gives safe pieces).
  // At the root, best_out receives the move that raised alpha, if any.
  Score negamax(Position &pos, Score alpha, Score beta,
                Move *best_out = nullptr);
  // negamax for the root of a solve, which may also have nothing in hand.
  // `best` receives the move that raised alpha, or EMPTY_MOVE.
  Score search_root(Position &pos, Score alpha, Score beta, Move &best);

public:
  explicit Solver(TranspositionTable &tt) : tt_(tt) {}
  // Once *stop is true, solve() returns as soon as it can, with a meaningless
  // result, and stores nothing more in the TT, so the TT stays correct.
  void set_stop(const std::atomic<bool> *stop) { stop_ = stop; }
  // Clear the TT after changing this: the two kinds of key must not meet.
  void set_canon_max_pieces(int n) { canon_max_pieces_ = n; }
  // `pos` is modified during the solve and restored before returning.
  SolveResult solve(Position &pos);
  // Solves the empty board on `threads` threads sharing the TT: each solves
  // positions a few turns in, and their values are combined. Prints a line
  // per finished job.
  //
  // `book_path` is an opening book and a checkpoint at once. Each solved job
  // is appended to it as it finishes, and jobs already in it are skipped, so
  // an interrupted solve resumes where it stopped. Wins and losses without a
  // distance (an older book) are solved again. At the end the file is
  // rewritten with every position from the empty board down to the jobs.
  // One position per line: cols and occ in hex, then value, cell and give,
  // the move in the canonical frame.
  SolveResult full_solve(unsigned threads = std::thread::hardware_concurrency(),
                         const std::string &book_path = "book.txt");
  // Nodes visited by the last solve.
  std::uint64_t nodes() const { return nodes_; }
};

} // namespace quarto
