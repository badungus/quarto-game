// assets.cpp - Implements loading and unloading of Assets from the assets/
// directory.
#include "assets.hpp"
#include <raylib.h>

void Assets::load() {
  pool_button_background = LoadTexture(
      TextFormat("%s../assets/images/buttons/pool_button_background.png",
                 GetApplicationDirectory()));
  pool_button_hover =
      LoadTexture(TextFormat("%s../assets/images/buttons/pool_button_hover.png",
                             GetApplicationDirectory()));
  pool_button_armed =
      LoadTexture(TextFormat("%s../assets/images/buttons/pool_button_armed.png",
                             GetApplicationDirectory()));
  for (int i = 0; i < 16; ++i) {
    pool_button_images[i] = LoadTexture(TextFormat(
        "%s../assets/images/pieces/piece%d.png", GetApplicationDirectory(), i));
  }
  winning_cell = LoadTexture(TextFormat(
      "%s../assets/images/board/winning_cell.png", GetApplicationDirectory()));
  board_background =
      LoadTexture(TextFormat("%s../assets/images/board/board_background.png",
                             GetApplicationDirectory()));
  restart = LoadTexture(TextFormat("%s../assets/images/buttons/restart.png",
                                   GetApplicationDirectory()));
  undo = LoadTexture(TextFormat("%s../assets/images/buttons/undo.png",
                                GetApplicationDirectory()));
  redo = LoadTexture(TextFormat("%s../assets/images/buttons/redo.png",
                                GetApplicationDirectory()));
  pause = LoadTexture(TextFormat("%s../assets/images/buttons/pause.png",
                                 GetApplicationDirectory()));
  resume = LoadTexture(TextFormat("%s../assets/images/buttons/resume.png",
                                  GetApplicationDirectory()));

  stop_search = LoadTexture(TextFormat(
      "%s../assets/images/buttons/stop_search.png", GetApplicationDirectory()));
  evalbar_background = LoadTexture(
      TextFormat("%s../assets/images/players/evalbar_background.png",
                 GetApplicationDirectory()));
  evalbar = LoadTexture(TextFormat("%s../assets/images/players/evalbar.png",
                                   GetApplicationDirectory()));
}

void Assets::unload() {
  UnloadTexture(pool_button_background);
  UnloadTexture(pool_button_hover);
  UnloadTexture(pool_button_armed);
  for (Texture2D &image : pool_button_images) {
    UnloadTexture(image);
  }
}
