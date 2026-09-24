#pragma once

#include "../game/game_state.hpp"

class Gui {
public:
  Gui(Game &game);
  void run();

private:
  Game &game_;
};
