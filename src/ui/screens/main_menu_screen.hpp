// main_menu_screen.hpp - Declares MainMenuScreen: title, New Game, Engine
// Match, Quit.

#pragma once
#include "assets.hpp"
#include "button.hpp"
#include "dropdown.hpp"
#include "game_state.hpp"
#include "input.hpp"
#include "match_config.hpp"
#include "match_controller.hpp"
#include "nav.hpp"
#include "text_field.hpp"
#include <array>
#include <variant>
#include <vector>

class MainMenu {
private:
  std::vector<Button> pool_buttons_;
  Game &game_;
  Button start_;
  Button engine_match_;
  Dropdown player_1_dd_;
  Dropdown player_2_dd_;
  Button rules_; // normal or 2x2 squares
  // Per player, one widget per ENGINE_SETTINGS entry.
  std::array<std::vector<std::variant<TextField, Button>>, 2> settings_ui_;
  MatchConfig &match_config_;

public:
  MainMenu(Game &game, MatchController &match_controller,
           MatchConfig &match_config, const Assets &assets);
  Action update(const Input &in);
  void draw_menu_buttons();
  void draw_dropdown();
};
