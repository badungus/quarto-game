// screen.cpp - Implements Screen switching.
#include "screen.hpp"
#include <optional>

Screen::Screen(const Assets &assets)
    : game_screen_(game, match_controller, assets) {}

void Screen::update(const Input &in) {
  switch (current_) {
  case ScreenId::Menu: {
    break;
  };
  case ScreenId::Game: {
    command = game_screen_.update(in);

    match_controller.update();
    break;
  }
  case ScreenId::Result: {

    break;
  }
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
