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
    pool_button_images[i] =
        LoadTexture(TextFormat("%s../assets/images/pieces/piece%d.png",
                               GetApplicationDirectory(), i + 1));
  }
}

void Assets::unload() {
  UnloadTexture(pool_button_background);
  UnloadTexture(pool_button_hover);
  UnloadTexture(pool_button_armed);
  for (Texture2D &image : pool_button_images) {
    UnloadTexture(image);
  }
}
