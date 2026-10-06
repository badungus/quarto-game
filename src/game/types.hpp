#pragma once
#include <array>
#include <cstdint>
#include <optional>
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

// One "info" line of a search. The typed fields are the keywords the GUI acts
// on and are empty when the engine did not send them. raw keeps the whole line
// so keywords the GUI does not know about can still be displayed.
struct EngineInfo {
  int depth = 0;
  std::optional<int16_t> score;
  std::optional<int> mate;
  std::optional<std::uint64_t> nodes;
  std::optional<std::string> move;
  std::string raw;
};

struct EngineInfoHistory {
  std::array<std::vector<EngineInfo>, 17> player_1_history;
  std::array<std::vector<EngineInfo>, 17> player_2_history;
};

constexpr int16_t WIN = 10000;
constexpr int16_t INF = 32000;
