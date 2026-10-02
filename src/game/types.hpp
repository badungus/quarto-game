#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

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

struct EngineInfo {
  int depth;
  int16_t score;
  std::string move;
};

struct EngineInfoHistory {
  std::array<std::vector<EngineInfo>, 17> player_1_history;
  std::array<std::vector<EngineInfo>, 17> player_2_history;
};

constexpr int16_t WIN = 10000;
constexpr int16_t INF = 32000;
