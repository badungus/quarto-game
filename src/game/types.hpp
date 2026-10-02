#pragma once
#include <array>
#include <cstdint>
#include <string>

using Piece = std::int8_t;
using Board = std::array<Piece, 16>;
using Cell = std::int8_t;
using Avail = std::array<bool, 16>;
struct Move {
  Cell cell;
  Piece give;
};
struct Undo {
  Cell cell;
  Piece hand;
};

struct EngineOutput {
  int depth;
  uint32_t score;
  std::string move;
};
