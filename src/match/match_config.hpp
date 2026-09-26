#pragma once
#include <array>
#include <string>
#include <variant>

struct EngineSeat {
  std::string engine_name;
  int search_depth;
  int time_limit;
};
struct HumanSeat {};

using Seat = std::variant<HumanSeat, EngineSeat>;

struct MatchConfig {
  Seat player_1;
  Seat player_2;
};
