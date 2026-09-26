// screen.cpp - Implements Screen switching.
#include "screen.hpp"

Screen::Screen(const Assets &assets)
    : game_screen_(game, assets), match_controller(game) {}

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
