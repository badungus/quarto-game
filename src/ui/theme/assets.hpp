// assets.hpp - Declares Assets: textures loaded once and owned by Gui.
#pragma once

#include <array>
#include <raylib.h>

struct Assets {
  Texture2D panel; // white fill, drawn with a tint from colors.hpp
  std::array<Texture2D, 16> pool_button_images;
  Texture2D winning_cell;
  Texture2D board_background;
  Texture2D background;
  Texture2D restart;
  Texture2D undo;
  Texture2D redo;
  Texture2D pause;
  Texture2D resume;
  Texture2D evalbar_background;
  Texture2D evalbar;
  Texture2D stop_search;

  // Must be called after InitWindow(), since textures need a GL context.
  void load();
  void unload();
};
