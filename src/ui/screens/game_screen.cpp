// game_screen.cpp - Implements GameScreen layout, input routing and reaction to
// match events.
#include "game_screen.hpp"
#include "../layout.hpp"
#include <raylib.h>

GameScreen::GameScreen(Game &game, const Assets &assets) : game_(game) {
  for (int i = 0; i < 16; ++i) {
    int col = i % 4;
    int row = i / 4;
    pool_buttons_.emplace_back(
        Rectangle{POOL_X + col * POOL_TILE, POOL_Y + row * POOL_TILE,
                  POOL_TILE, POOL_TILE},
        assets.pool_button_background, assets.pool_button_hover);
  }
}

void GameScreen::update(const Input &in) {
  for (int i = 0; i < 16; ++i) {
    if (game_.avail()[i])
      pool_buttons_[i].update(in);
  }
}

void GameScreen::draw_piece_pool() {
  for (int i = 0; i < 16; ++i) {
    if (game_.avail()[i])
      pool_buttons_[i].draw();
  }
}
