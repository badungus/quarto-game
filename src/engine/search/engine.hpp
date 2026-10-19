#pragma once
#include "position.hpp"
#include "features.hpp"
#include "search.hpp"
#include "tt.hpp"
#include "types.hpp"
#include <array>
#include <climits>
#include <cstdint>
#include <functional>
#include <optional>
#include <thread>
#include <tuple>

namespace quarto {

const int DEFAULT_MOVETIME = INT_MAX;
const bool DEFAULT_INFINITE = false;
const bool DEFAULT_SQUARES = false; // normal play: only lines win
const int DEFAULT_DEPTH = 16;
// TT size in MB; without the TT feature the table is not used, so it gets the
// smallest size, one entry.
const int DEFAULT_TT_SIZE = TT ? 2048 : 0;
const std::tuple<Bitboard, std::array<Bitboard, 4>, Piece> DEFAULT_POSITION = {
    0xffff, {0, 0, 0, 0}, -1};

struct SearchLimits {
  int depth;
  int movetime;
  bool infinite;
};

struct SearchCallbacks {
  InfoFn on_info;
  std::function<void(Result)> on_bestmove;
  // Called before the search starts when the seed has not been reported yet.
  std::function<void(std::uint64_t)> on_seed;
};

class Engine {
private:
  int depth_ = DEFAULT_DEPTH;
  int movetime_ = DEFAULT_MOVETIME;
  bool infinite_ = DEFAULT_INFINITE;
  bool squares_ = DEFAULT_SQUARES;
  // Applied in go(), once the previous search thread has finished, so the
  // generator is never reseeded while the search thread uses it.
  std::optional<std::uint64_t> pending_seed_;
  // The startup seed counts as new, so the first go reports it too.
  bool seed_reported_ = false;
  Position pos_;
  TranspositionTable tt_; // declared before search_, which refers to it
  Search search_;
  std::jthread search_thread_; // declared last: joins before the rest is gone

public:
  Engine();
  ~Engine() { stop(); }
  void stop();
  void set_position(std::tuple<Bitboard, std::array<Bitboard, 4>, Piece> pos);
  void go(SearchCallbacks callback);
  void set_depth(int depth) { depth_ = depth; }
  void set_movetime(int movetime) { movetime_ = movetime; }
  void set_infinite(bool infinite) { infinite_ = infinite; }
  void set_squares(bool squares) { squares_ = squares; }
  void set_seed(std::uint64_t seed) { pending_seed_ = seed; }
};
} // namespace quarto
