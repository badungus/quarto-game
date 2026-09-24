// game_screen.cpp - Implements GameScreen layout, input routing and reaction to
// match events.
#include "game_screen.hpp"
#include "../layout.hpp"
#include "../widgets/button.hpp"
#include <array>
#include <raylib.h>
#include <vector>

GameScreen::GameScreen(Game &game) : game_(game) {
  Texture2D pool_button_image = LoadTexture(
      TextFormat("%s../assets/images/buttons/pool_button_background.png",
                 GetApplicationDirectory()));

  for (int i = 0; i < 16; ++i) {
    button_images[i] =
        LoadTexture(TextFormat("%s../assets/images/pieces/piece%d.png",
                               GetApplicationDirectory(), i + 1));

    int col = i % 4;
    int row = i / 4;

    pool_buttons.emplace_back(PIECEPOOLX + (col * POOLBUTTONSIZE),
                              PIECEPOOLY + (row * POOLBUTTONSIZE),
                              POOLBUTTONSIZE, POOLBUTTONSIZE, button_images[i],
                              pool_button_image);
  }
}
void GameScreen::draw_piece_pool() {
  for (int i = 0; i < 16; ++i) {
    if (game_.avail()[i])
      pool_buttons[i].draw();
  };
};
