// results_screen.hpp - Declares EngineMatch: plays a series of games between
// two engines like tools/match.py, alternating who moves first, and shows each
// game's result, the score and an Elo estimate.
#pragma once
#include "assets.hpp"
#include "button.hpp"
#include "command.hpp"
#include "dropdown.hpp"
#include "game_state.hpp"
#include "input.hpp"
#include "match_config.hpp"
#include "match_controller.hpp"
#include "nav.hpp"
#include "text_field.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <variant>
#include <vector>

class EngineMatch {
private:
  Game &game_;
  MatchController &match_controller_;
  MatchConfig &match_config_;
  const Assets &assets_;
  Button start_;
  TextField games_field_;
  TextField seed_field_;
  Dropdown player_1_dd_;
  Dropdown player_2_dd_;
  // Per engine, one widget per ENGINE_SETTINGS entry.
  std::array<std::vector<std::variant<TextField, Button>>, 2> settings_ui_;

  // Engine A and B: index into ENGINES and the search settings of each. They
  // swap seats every game, so A moves first in even games.
  std::array<int, 2> engines_{0, 1};
  std::array<std::vector<std::string>, 2> settings_{default_settings(),
                                                    default_settings()};

  bool running_ = false;
  MatchConfig saved_config_; // the main menu's config, restored afterwards
  std::mt19937 rng_;         // draws the engine seeds of every game
  std::array<std::uint32_t, 2> seeds_{}; // of the game in progress, per seat
  int max_games_ = 0;
  int games_played_ = 0;
  std::array<int, 2> wins_{}; // engine A, engine B
  int draws_ = 0;
  int first_wins_ = 0;
  int second_wins_ = 0;
  int total_length_ = 0; // placements over all games
  std::vector<std::string> log_;
  int scroll_ = 0; // lines scrolled up from the end of the log

  const std::string &name(int engine) const {
    return ENGINES[engines_[engine]].name;
  }
  int first_engine() const { return games_played_ % 2; }
  void log(std::string line);
  std::optional<Command> begin();
  Command start_game();
  void record(const GameResult &result);
  void end();
  void draw_output() const;

public:
  EngineMatch(Game &game, MatchController &match_controller,
              MatchConfig &match_config, const Assets &assets);
  Action update(const Input &in);
  void draw();
  // Abandons the match, e.g. because the screen was left.
  void stop();
  bool running() const { return running_; }
};
