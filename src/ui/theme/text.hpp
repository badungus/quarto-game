// text.hpp - Declares text drawing for the UI, so all text uses one font.
#pragma once

#include <raylib.h>

namespace text {

// Set by Assets::load(). Until then, raylib's default font is used.
inline Font font = {};

inline Font current() { return font.texture.id != 0 ? font : GetFontDefault(); }

// Like DrawText, but in the UI font. (x, y) is the top left, size the height.
inline void draw(const char *text, float x, float y, float size, Color color) {
  DrawTextEx(current(), text, {x, y}, size, 0.0f, color);
}

// Like MeasureText, but in the UI font.
inline float measure(const char *text, float size) {
  return MeasureTextEx(current(), text, size, 0.0f).x;
}

} // namespace text
