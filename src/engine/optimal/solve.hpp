#pragma once
#include "position.hpp"
#include "tt.hpp"
#include "types.hpp"
#include <atomic>
#include <cstdint>
#include <array>
#include <map>
#include <string>
#include <thread>
#include <unordered_set>
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

// A canonical position: cols and occ of its Canon. With piece 0 in hand it is
// a position itself, equivalent to every position it is the key of: load it
// with canonical_position.
using CanonKey = std::pair<std::uint64_t, Bitboard>;
Position canonical_position(const CanonKey &key);
struct CanonKeyHash {
  std::size_t operator()(const CanonKey &key) const {
    // A multiply-xorshift mix: cols alone are close to sequential.
    std::uint64_t h = (key.first ^ key.second) * 0x9E3779B97F4A7C15ull;
    return std::size_t(h ^ (h >> 32));
  }
};

// A book position: its value and every move achieving it, in the canonical
// frame (from_canonical maps each into any equivalent position). Bit g of
// gives[c] is set when placing on cell c and giving piece g is best. When
// every move loses at once, every move is set.
struct BookEntry {
  Score value;
  std::array<Bitboard, 16> gives;
};
using Book = std::map<CanonKey, BookEntry>;

// The opening book written by Solver::full_solve, a binary file: BOOK_MAGIC,
// then one record per position, in key order. A record is cols (8 bytes), occ
// (2), value (2), a mask of the cells with a best move (2), and gives[c] (2
// each) for the cells in that mask, lowest first; all little-endian. A missing
// file is an empty book.
//
// A file that does not start with BOOK_MAGIC is read as the older text book
// (book.txt): one position per line, cols and occ in hex, then value, cell
// and give, one best move each. Its wins and losses are 1 and -1, with no
// distance. Lines that do not parse (comments start with '#') are skipped.
inline constexpr char BOOK_MAGIC[8] = {'Q', 'B', 'O', 'O', 'K', 'v', '2', 0};
Book read_book(const std::string &path);
// Writes `book` to `path` at once: to a new file first, then renamed over the
// old one, so at every moment there is a complete book.
void write_book(const std::string &path, const Book &book);

class Solver {
private:
  TranspositionTable &tt_;
  std::uint64_t nodes_ = 0;
  // Set from another thread to abandon the solve; null for never.
  const std::atomic<bool> *stop_ = nullptr;
  bool stopped() const {
    return stop_ && stop_->load(std::memory_order_relaxed);
  }

  // A full-solve job: its canonical key, and the position as the search
  // first reached it, which is what gets solved.
  struct Job {
    CanonKey key;
    std::uint64_t props; // the four property boards, 16 bits each
    Bitboard occ;
    Piece hand;
  };
  // The full solve's jobs: every position SEARCH_START_PLY turns in, one
  // per group of equivalent positions, in the order they are found.
  std::vector<Job> create_jobs_();
  void find_jobs(int ply, Position &pos,
                 std::unordered_set<CanonKey, CanonKeyHash> &seen,
                 std::vector<Job> &jobs);
  // Value of `pos` for the side to move, `ply` turns below the empty board,
  // from the values of the jobs, sorted by key. Adds every position above
  // the jobs to the book, with all its best moves; a position already in it
  // is not combined again.
  Score combine(int ply, Position &pos,
                const std::vector<std::pair<CanonKey, Score>> &values,
                Book &book);

  // Positions with at most this many pieces are keyed by their canonical
  // form, so equivalent positions share a TT entry; deeper ones by the
  // position as it is. The piece count decides, so a position always gets the
  // same kind of key, and the two kinds never meet. -1 turns canonical keys
  // off, which only the check program does. 11 by benchmark: on ply-6 jobs,
  // 3.5-6% faster than 10, and 12 or 9 are slower than both.
  int canon_max_pieces_ = 11;

  // False skips the second search: wins and losses then score 1 and -1, with
  // no distance, as in a book written before solves had distances.
  bool distances_ = true;

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
  void set_distances(bool on) { distances_ = on; }
  // `pos` is modified during the solve and restored before returning.
  SolveResult solve(Position &pos);
  // Solves the empty board on `threads` threads sharing the TT: each solves
  // positions a few turns in (the jobs), and their values are combined.
  // Prints a line per finished job.
  //
  // The book written to `book_path` holds every position from the empty
  // board down to the turn before the jobs, with all its best moves; the
  // jobs give the values and are not in it. Their values go to a checkpoint,
  // `book_path` + ".jobs", appended as each job finishes, one per line: cols
  // and occ in hex, then the value. Jobs already in it are skipped, so an
  // interrupted solve resumes where it stopped; with distances on, a win or
  // loss saved without one is solved again. Once the book is written the
  // checkpoint is deleted.
  SolveResult full_solve(unsigned threads = std::thread::hardware_concurrency(),
                         const std::string &book_path = "book.bin");
  // Nodes visited by the last solve.
  std::uint64_t nodes() const { return nodes_; }
};

} // namespace quarto
