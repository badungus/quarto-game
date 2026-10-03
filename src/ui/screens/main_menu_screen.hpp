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
#include <vector>

class MainMenu {
private:
  std::vector<Button> pool_buttons_;
  Game &game_;
  Button start_;
  Dropdown player_1_dd_;
  Dropdown player_2_dd_;
  MatchConfig &match_config_;

public:
  MainMenu(Game &game, MatchController &match_controller,
           MatchConfig &match_config, const Assets &assets);
  Action update(const Input &in);
  void draw_menu_buttons();
  void draw_dropdown();
};
