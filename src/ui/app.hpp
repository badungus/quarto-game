#pragma once

#include "../game/game_state.hpp"
#include "screens/screen.hpp"
#include "theme/assets.hpp"

class Gui {
public:
  Gui(Game &game);
  void run();

private:
  Assets assets_; // declared before screen_, which refers to it
  Screen screen_;
};
