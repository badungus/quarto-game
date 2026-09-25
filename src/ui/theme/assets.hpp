// assets.hpp - Declares Assets: textures loaded once and owned by Gui.
#pragma once

#include <array>
#include <raylib.h>

struct Assets {
  Texture2D pool_button_background;
  Texture2D pool_button_hover;
  Texture2D pool_button_armed;
  std::array<Texture2D, 16> pool_button_images;

  // Must be called after InitWindow(), since textures need a GL context.
  void load();
  void unload();
};
