#pragma once
#include "position.hpp"
#include "search.hpp"
#include "types.hpp"
#include <array>
#include <climits>
#include <functional>
#include <tuple>

namespace quarto {

const int DEFAULT_MOVETIME = INT_MAX;
const bool DEFAULT_INFINITE = false;
const int DEFAULT_DEPTH = 16;
const std::tuple<Bitboard, std::array<Bitboard, 4>, Piece> DEFAULT_POSITION = {
    0xffff, {0, 0, 0, 0}, -1};

struct SearchLimits {
  int depth;
  int movetime;
  bool infinite;
};

struct SearchCallbacks {
  InfoFn on_info;
  std::function<void(Move)> on_bestmove;
};

class Engine {
private:
  int depth_ = DEFAULT_DEPTH;
  int movetime_ = DEFAULT_MOVETIME;
  bool infinite_ = DEFAULT_INFINITE;
  Position pos_;
  Search search_;

public:
  Engine() { set_position(DEFAULT_POSITION); }
  void stop();
  void set_position(std::tuple<Bitboard, std::array<Bitboard, 4>, Piece> pos);
  void go(SearchCallbacks callback);
  void set_depth(int depth) { depth_ = depth; }
  void set_movetime(int movetime) { movetime_ = movetime; }
  void set_infinite(bool infinite) { infinite_ = infinite; }
};
} // namespace quarto
