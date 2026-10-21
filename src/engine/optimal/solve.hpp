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

// Proves if the side to move wins, draws or loses with perfect play, and how
// soon. A solve first finds the result, then the exact score of a win or loss.

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

// The value as UQI mate, 0 for a draw or a value without a distance.
int mate_distance(Score value, const Position &pos);

struct SolveResult {
  Score value; // the score for the side to move, as above
  // A move that gets value. When every move loses, one that loses as late as
  // possible.
  Move move;
};

// A canonical position. With piece 0 in hand it can be loaded with
// canonical_position.
using CanonKey = std::pair<std::uint64_t, Bitboard>;
Position canonical_position(const CanonKey &key);
struct CanonKeyHash {
  std::size_t operator()(const CanonKey &key) const {
    // A multiply-xorshift mix: cols alone are close to sequential.
    std::uint64_t h = (key.first ^ key.second) * 0x9E3779B97F4A7C15ull;
    return std::size_t(h ^ (h >> 32));
  }
};

// A book position: its value and every best move in the canonical frame.
// Bit g of gives[c] is set when placing on cell c and giving piece g is best.
struct BookEntry {
  Score value;
  std::array<Bitboard, 16> gives;
};
using Book = std::map<CanonKey, BookEntry>;

// The binary opening book written by Solver::full_solve. A missing file is an
// empty book.
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
  // Value of pos from the values of the jobs. Adds every position above the
  // jobs to the book, with all its best moves.
  Score combine(int ply, Position &pos,
                const std::vector<std::pair<CanonKey, Score>> &values,
                Book &book);

  // Positions with at most this many pieces use canonical TT keys. -1 turns
  // canonical keys off.
  int canon_max_pieces_ = 11;

  // False skips the second search: wins and losses then score 1 and -1, with
  // no distance.
  bool distances_ = true;

  // Fail-hard alpha-beta value of pos for the side to move. The piece in hand
  // cannot win. At the root, best_out gets the move that raised alpha.
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
  // Solves the empty board on many threads and writes the book. Finished jobs
  // are saved to book_path.jobs, so an interrupted solve can resume.
  SolveResult full_solve(unsigned threads = std::thread::hardware_concurrency(),
                         const std::string &book_path = "book.bin");
  // Nodes visited by the last solve.
  std::uint64_t nodes() const { return nodes_; }
};

} // namespace quarto
