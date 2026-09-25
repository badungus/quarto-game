// screen.cpp - Implements Screen switching.
#include "screen.hpp"

Screen::Screen(Game &game, const Assets &assets)
    : game_(game), game_screen_(game, assets) {}

void Screen::update(const Input &in) {
  switch (current_) {
  case ScreenId::Game:
    game_screen_.update(in);
    break;
  default:
    break;
  }
}

void Screen::draw() {
  switch (current_) {
  case ScreenId::Game:
    game_screen_.draw_piece_pool();
    game_screen_.draw_board();
    break;
  default:
    break;
  }
}
