#pragma once
#include "position.hpp"
#include "tt.hpp"
#include "types.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <random>
#include <vector>

namespace quarto {

// The engine's search: position in -> best move out.
//
// search(pos, depth, cb) runs iterative deepening from depth 1 up to `depth`,
// and stops early once the movetime budget is spent or set_stop(true) is
// called. An interrupted iteration is discarded, so the result always comes
// from the deepest *completed* depth. `cb` (may be empty) is called once per
// completed depth.
//
// Returns {score, move}, where move is {cell, give}:
//   cell  0..15 board index (row * 4 + col) where the held piece is placed, or
//         -1 at game start (give-only move)
//   give  the piece handed to the opponent, or -1 if the game ends
//
// `pos` is modified during the search and restored before returning.

struct Result {
  Score score;
  Move move;
};
struct SearchInfo {
  int depth;
  Result result;
  std::uint64_t nodes; // nodes visited so far, over all iterations
  // Forced result in turns of the side to move (one turn = place + give):
  // N > 0 wins on its own N-th turn, N < 0 loses on the opponent's N-th turn,
  // 0 if nothing is proven yet.
  int mate;
};
using InfoFn = std::function<void(const SearchInfo &)>;

class Search {
private:
  std::uint64_t nodes_; // nodes visited this search
  std::chrono::steady_clock::time_point start_time_; // when search() began
  int movetime_;                                     // time budget in ms
  // Set when time runs out or on request (from another thread); aborts search.
  std::atomic<bool> stop_ = false;
  // Picks among tied best moves. Kept across searches, so a fixed seed makes
  // a whole game reproducible (with fixed depth; movetime adds timing noise).
  // seed_ is declared first because gen_ is initialised from it.
  std::uint64_t seed_ = std::random_device{}();
  std::mt19937_64 gen_{seed_};
  TranspositionTable &tt_; // owned by Engine, shared across searches

  // Scores every legal root move at `depth` and returns all moves tied for
  // the best score (never empty).
  std::vector<Result> negamax_root(Position &pos, int depth);
  // Fail-hard alpha-beta score of `pos` for the side to move.
  Score negamax(Position &, int depth, Score alpha, Score beta);

public:
  Search(TranspositionTable &tt) : tt_(tt) {};
  Result search(Position &pos, int depth, InfoFn cb);
  // Time budget for the next search(), in milliseconds.
  void set_movetime(int movetime) { movetime_ = movetime; }
  // Ask a running search to stop; it returns its last completed result.
  void set_stop(bool stop) { stop_ = stop; }
  // Reseeds the tie-break generator. Only call while no search is running.
  void set_seed(std::uint64_t seed) {
    seed_ = seed;
    gen_.seed(seed);
  }
  // The seed the generator was last started from.
  std::uint64_t seed() const { return seed_; }
};
} // namespace quarto
