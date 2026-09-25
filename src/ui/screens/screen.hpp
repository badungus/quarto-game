// screen.hpp - Declares Screen: owns the screens, tracks which one is active
// and forwards update(input) and draw() to it.
#pragma once
#include "../../game/game_state.hpp"
#include "../input.hpp"
#include "../theme/assets.hpp"
#include "game_screen.hpp"

enum class ScreenId { Menu, Game, Result };

class Screen {
private:
  Game &game_;
  GameScreen game_screen_;
  ScreenId current_ = ScreenId::Game;

public:
  Screen(Game &game, const Assets &assets);
  void update(const Input &in);
  void draw();
};
