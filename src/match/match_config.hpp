#pragma once
#include <string>
#include <variant>

struct EngineSeat {
  std::string name;
  int search_depth;
  int time_limit;
};
struct HumanSeat {
  std::string name;
};

using Seat = std::variant<HumanSeat, EngineSeat>;

struct MatchConfig {
  Seat player_1;
  Seat player_2;
};
