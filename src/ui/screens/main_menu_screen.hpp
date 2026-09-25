// main_menu_screen.hpp - Declares MainMenuScreen: title, New Game, Engine
// Match, Quit.

#pragma once
#include "../../game/game_state.hpp"
#include "../input.hpp"
#include "../theme/assets.hpp"
#include "../widgets/button.hpp"
#include <vector>

class MainMenu {
private:
  std::vector<Button> pool_buttons_;
  Game &game_;

public:
  MainMenu(Game &game, const Assets &assets);
  void update(const Input &in);
  void draw_menu_buttons();
};
