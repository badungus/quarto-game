// main_menu_screen.hpp - Declares MainMenuScreen: title, New Game, Engine
// Match, Quit.

#pragma once
#include "game_state.hpp"
#include "match_controller.hpp"
#include "input.hpp"
#include "assets.hpp"
#include "button.hpp"
#include <vector>

class MainMenu {
private:
  std::vector<Button> pool_buttons_;
  Game &game_;

public:
  MainMenu(Game &game, MatchController &match_controller, const Assets &assets);
  void update(const Input &in);
  void draw_menu_buttons();
};
