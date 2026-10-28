// assets.hpp - Declares Assets: textures loaded once and owned by Gui.
#pragma once

#include <array>
#include <raylib.h>

struct Assets {
  Texture2D panel;   // white fill, drawn with a tint from colors.hpp
  Texture2D outline; // white ring, same shape as panel, empty inside
  std::array<Texture2D, 16> pool_button_images;
  // One-colour textures, loaded white so they can be tinted. evalbar is
  // white already.
  Texture2D background;
  Texture2D restart;
  Texture2D undo;
  Texture2D redo;
  Texture2D pause;
  Texture2D resume;
  Texture2D evalbar_background;
  Texture2D evalbar;
  Texture2D stop_search;
  Font font; // Chakra Petch, used for all text through text.hpp

  // Must be called after InitWindow(), since textures need a GL context.
  void load();
  void unload();
};
