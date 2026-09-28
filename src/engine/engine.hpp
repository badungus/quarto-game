#pragma once
#include "position.hpp"
#include "types.hpp"
#include <array>
#include <functional>
#include <tuple>

namespace quarto {

struct SearchLimits {
  int depth = -1;
  int move_time = -1;
  bool infinite = false;
};
class Engine {
private:
  int depth_;
  int search_time_;
  Position pos_;

public:
  void stop();
  void set_position(std::tuple<Bitboard, std::array<Bitboard, 4>, Piece> pos);
  void go(SearchLimits limits, std::function<void(Move)> callback);
};
} // namespace quarto
