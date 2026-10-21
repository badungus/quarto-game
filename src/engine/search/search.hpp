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

// The engine's search: position in, best move out

struct Result {
  Score score;
  Move move;
};
struct SearchInfo {
  int depth;
  Result result;
  std::uint64_t nodes; // nodes visited so far, over all iterations
  int mate;
};
using InfoFn = std::function<void(const SearchInfo &)>;

class Search {
private:
  std::uint64_t nodes_; // nodes visited this search
  std::chrono::steady_clock::time_point start_time_; // when search() began
  int movetime_;                                     // time budget in ms
  std::atomic<bool> stop_ = false;
  std::uint64_t seed_ = std::random_device{}();
  std::mt19937_64 gen_{seed_};
  TranspositionTable &tt_; // owned by Engine, shared across searches
  std::vector<Result> negamax_root(Position &pos, int depth, Move first);
  Score negamax(Position &, int depth, Score alpha, Score beta);

public:
  Search(TranspositionTable &tt) : tt_(tt) {};
  Result search(Position &pos, int depth, InfoFn cb);
  // Time budget for the next search in milliseconds.
  void set_movetime(int movetime) { movetime_ = movetime; }
  // Ask a running search to stop, it returns its last completed result.
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
