// button.cpp - Implements Button drawing and click handling using Theme tokens.
#include "button.hpp"
#include "../layout.hpp"
#include <raylib.h>

Button::Button(int x, int y, int width, int height, Texture2D image,
               Texture2D background)
    : x(x), y(y), width(width), height(height), image(image),
      background(background) {};

void Button::draw() {
  float s = ui_scale();
  DrawTexturePro(image, {0, 0, (float)image.width, (float)image.height},
                 {x * s, y * s, width * s, height * s}, {0, 0}, 0.0f, WHITE);
};
