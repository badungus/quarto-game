#pragma once
#include <array>
#include <numbers>
#include <string>
#include <variant>

struct EngineSeat {
  std::string name;
  std::string executable;
  int search_depth;
  int time_limit;
};

inline constexpr std::array ENGINES{
    EngineSeat{"Thor", "thor", 16, 1000},
    EngineSeat{"Odin", "odin", 16, 1000},
};
inline constexpr std::array ENGINE_NAMES = {ENGINES[0].name, ENGINES[1].name};

constexpr auto make_players() {
  std::array<std::string, ENGINES.size() + 1> result{"PLAYER"};

  for (std::size_t i = 0; i < ENGINES.size(); ++i)
    result[i + 1] = ENGINES[i].name;

  return result;
}

inline constexpr auto PLAYERS = make_players();

inline constexpr int ENGINE_COUNT = ENGINES.size();

struct HumanSeat {
  std::string name;
};

using Seat = std::variant<HumanSeat, EngineSeat>;

struct MatchConfig {
  Seat player_1;
  Seat player_2;
};
