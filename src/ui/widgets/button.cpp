// button.cpp - Implements Button drawing and click handling.
#include "button.hpp"
#include "../layout.hpp"
#include <raylib.h>

Button::Button(Rectangle bounds, const Texture2D &background,
               const Texture2D &hover_image)
    : bounds_(bounds), background_(background), hover_image_(hover_image) {}

bool Button::update(const Input &in) {
  hover_ = CheckCollisionPointRec(in.mouse, bounds_);
  return hover_ && in.pressed;
}

void Button::draw() const {
  float s = ui_scale();
  const Texture2D &tex = hover_ ? hover_image_ : background_;
  DrawTexturePro(tex, {0, 0, (float)tex.width, (float)tex.height},
                 {bounds_.x * s, bounds_.y * s, bounds_.width * s,
                  bounds_.height * s},
                 {0, 0}, 0.0f, WHITE);
}
