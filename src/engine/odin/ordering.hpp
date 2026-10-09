#pragma once
#include "position.hpp"
#include "types.hpp"
#include <array>

namespace quarto {

struct ScoredMove {
  Move move;
  Score score;
};

struct MoveList {
  std::array<ScoredMove, 240> moves;
  int size;
};

void generate(Position &pos, MoveList &list);

}; // namespace quarto
