// main_menu_screen.cpp - Implements MainMenuScreen.

#include "main_menu_screen.hpp"
#include "../layout.hpp"
#include <raylib.h>

MainMenu::MainMenu(Game &game, const Assets &assets) : game_(game) {
  for (int i = 0; i < 16; ++i) {
    int col = i % 4;
    int row = i / 4;
    pool_buttons_.emplace_back(
        Rectangle{POOL_X + col * POOL_TILE, POOL_Y + row * POOL_TILE, POOL_TILE,
                  POOL_TILE},
        assets.pool_button_background, assets.pool_button_hover,
        &assets.pool_button_images[i]);
  }
}

void MainMenu::update(const Input &in) {}

void MainMenu::draw_menu_buttons() {}
