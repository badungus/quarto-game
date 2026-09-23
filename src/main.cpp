#include "game/game_state.hpp"
#include "ui/app.hpp"

int main() {
  Game game;
  Gui gui(game);

  gui.run();

  return 0;
}
