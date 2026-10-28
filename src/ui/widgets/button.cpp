// button.cpp - Implements Button drawing and click handling.
#include "button.hpp"
#include "colors.hpp"
#include "layout.hpp"
#include <raylib.h>
#include <string>
#include <variant>

Button::Button(Rectangle bounds, const Texture2D &panel, ButtonContent content,
               float text_scale)
    : bounds_(bounds), panel_(panel), content_(content),
      text_scale_(text_scale) {}

bool Button::update(const Input &in) {
  hover_ = CheckCollisionPointRec(in.mouse, bounds_) && enabled_;
  armed_ = hover_ && (in.is_pressed || in.released);
  return armed_ && in.released;
}

void Button::draw() const {
  float s = ui_scale();
  const Texture2D &tex = panel_;
  const Color tint = armed_   ? colors::PANEL_ARMED
                     : hover_ ? colors::PANEL_HOVER
                              : colors::PANEL;
  Rectangle dest = {bounds_.x * s, bounds_.y * s, bounds_.width * s,
                    bounds_.height * s};
  // 9-slice: corners keep their size, edges and centre stretch. The border
  // covers the outline and rounded corners of the button textures.
  constexpr int BORDER = 20;
  NPatchInfo patch = {{0, 0, (float)tex.width, (float)tex.height},
                      BORDER,
                      BORDER,
                      BORDER,
                      BORDER,
                      NPATCH_NINE_PATCH};
  DrawTextureNPatch(tex, patch, dest, {0, 0}, 0.0f, tint);

  if (std::holds_alternative<std::string>(content_)) {
    const std::string &text = std::get<std::string>(content_);
    int font_size = (int)(dest.height * 0.3f * text_scale_);
    int text_width = MeasureText(text.c_str(), font_size);
    DrawText(text.c_str(), (int)(dest.x + (dest.width - text_width) / 2),
             (int)(dest.y + (dest.height - font_size) / 2), font_size,
             colors::OUTLINE);
  } else if (std::holds_alternative<const Texture2D *>(content_)) {
    const Texture2D &image = *std::get<const Texture2D *>(content_);
    DrawTexturePro(image, {0, 0, (float)image.width, (float)image.height}, dest,
                   {0, 0}, 0, WHITE);
  }
}
