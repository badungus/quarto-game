// assets.cpp - Implements loading and unloading of Assets from the assets/
// directory.
#include "assets.hpp"
#include "text.hpp"
#include <raylib.h>

namespace {
// Loads a one-colour image as white, keeping only its alpha, so it can be
// tinted to any palette colour. A tint only darkens, so the image's own colour
// would cap what the tint can reach.
Texture2D load_mask(const char *path) {
  Image image = LoadImage(path);
  ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  Color *pixels = (Color *)image.data;
  for (int i = 0; i < image.width * image.height; ++i)
    pixels[i] = {255, 255, 255, pixels[i].a};
  Texture2D texture = LoadTextureFromImage(image);
  UnloadImage(image);
  return texture;
}
} // namespace

void Assets::load() {
  panel = LoadTexture(TextFormat("%s../assets/images/background/panel.png",
                                 GetApplicationDirectory()));
  outline = LoadTexture(TextFormat("%s../assets/images/background/outline.png",
                                   GetApplicationDirectory()));
  for (int i = 0; i < 16; ++i) {
    pool_button_images[i] = LoadTexture(TextFormat(
        "%s../assets/images/pieces/piece%d.png", GetApplicationDirectory(), i));
  }
  restart = load_mask(TextFormat("%s../assets/images/buttons/restart.png",
                                 GetApplicationDirectory()));
  undo = load_mask(TextFormat("%s../assets/images/buttons/undo.png",
                              GetApplicationDirectory()));
  redo = load_mask(TextFormat("%s../assets/images/buttons/redo.png",
                              GetApplicationDirectory()));
  pause = load_mask(TextFormat("%s../assets/images/buttons/pause.png",
                               GetApplicationDirectory()));
  resume = load_mask(TextFormat("%s../assets/images/buttons/resume.png",
                                GetApplicationDirectory()));

  stop_search = load_mask(TextFormat(
      "%s../assets/images/buttons/stop_search.png", GetApplicationDirectory()));
  evalbar_background =
      load_mask(TextFormat("%s../assets/images/players/evalbar_background.png",
                           GetApplicationDirectory()));
  evalbar = LoadTexture(TextFormat("%s../assets/images/players/evalbar.png",
                                   GetApplicationDirectory()));

  // Rasterized large and drawn smaller, so mipmaps keep small text smooth.
  font = LoadFontEx(TextFormat("%s../assets/fonts/ChakraPetch-SemiBold.ttf",
                               GetApplicationDirectory()),
                    96, nullptr, 0);
  GenTextureMipmaps(&font.texture);
  SetTextureFilter(font.texture, TEXTURE_FILTER_TRILINEAR);
  text::font = font;
}

void Assets::unload() {
  UnloadTexture(panel);
  UnloadTexture(outline);
  text::font = {};
  UnloadFont(font);
  for (Texture2D &image : pool_button_images) {
    UnloadTexture(image);
  }
}
