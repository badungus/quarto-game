#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct EngineSeat {
  std::string name;
  std::string executable;
  // Squares mode is a startup flag (--squares) for this engine, not a go
  // option: optimal picks its opening book when it starts.
  bool squares_flag = false;
};

inline constexpr std::array ENGINES{
    EngineSeat{"Thor", "thor"},
    EngineSeat{"Odin", "odin"},
    EngineSeat{"Baldur", "baldur"},
    EngineSeat{"Optimal", "optimal", true},
};
constexpr auto make_engine_names() {
  std::array<std::string, ENGINES.size()> result{};

  for (std::size_t i = 0; i < ENGINES.size(); ++i)
    result[i] = ENGINES[i].name;

  return result;
}

inline constexpr auto ENGINE_NAMES = make_engine_names();

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

// Engine search settings, set per player in the main menu. A Number setting
// sends "token value", a Toggle sends "token" while on; empty values are left
// out of the "go" command.
enum class SettingKind { Number, Toggle };

struct EngineSetting {
  std::string label;
  std::string token;
  SettingKind kind;
  std::string default_value;
};

inline const std::vector<EngineSetting> ENGINE_SETTINGS = {
    {"DEPTH", "depth", SettingKind::Number, "16"},
    {"MOVETIME", "movetime", SettingKind::Number, "3000"},
    {"INFINITE", "infinite", SettingKind::Toggle, ""},
};

inline std::vector<std::string> default_settings() {
  std::vector<std::string> values;
  for (const EngineSetting &s : ENGINE_SETTINGS)
    values.push_back(s.default_value);
  return values;
}

struct MatchConfig {
  Seat player_1;
  Seat player_2;
  // Value per entry of ENGINE_SETTINGS, for player one and player two.
  std::array<std::vector<std::string>, 2> settings{default_settings(),
                                                   default_settings()};
  // Sent to the engines as "seed N" before the game; unset leaves the engine
  // with its own random seed.
  std::array<std::optional<std::uint64_t>, 2> seeds{};
  // 2x2 squares of four pieces sharing a property win too, for both players.
  bool squares = false;

  std::string go_args(int player) const {
    std::string args;
    const auto *engine = std::get_if<EngineSeat>(player == 0 ? &player_1
                                                             : &player_2);
    if (squares && engine && !engine->squares_flag)
      args += " squares";
    for (size_t i = 0; i < ENGINE_SETTINGS.size(); ++i) {
      const std::string &value = settings[player][i];
      if (value.empty())
        continue;
      args += " " + ENGINE_SETTINGS[i].token;
      if (ENGINE_SETTINGS[i].kind == SettingKind::Number)
        args += " " + value;
    }
    return args;
  }
};
