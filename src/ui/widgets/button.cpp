// button.cpp - Implements Button drawing and click handling.
#include "button.hpp"
#include "../layout.hpp"
#include <raylib.h>
#include <string>
#include <variant>

Button::Button(Rectangle bounds, const Texture2D &background,
               const Texture2D &hover_image, const Texture2D &armed_image,
               ButtonContent content)
    : bounds_(bounds), background_(background), hover_image_(hover_image),
      armed_image_(armed_image), content_(content) {}

bool Button::update(const Input &in) {
  hover_ = CheckCollisionPointRec(in.mouse, bounds_) && enabled_;
  armed_ = hover_ && (in.is_pressed || in.released);
  return armed_ && in.released;
}

void Button::draw() const {
  float s = ui_scale();
  const Texture2D &tex = armed_   ? armed_image_
                         : hover_ ? hover_image_
                                  : background_;
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
  DrawTextureNPatch(tex, patch, dest, {0, 0}, 0.0f, WHITE);

  if (std::holds_alternative<std::string>(content_)) {
    const std::string &text = std::get<std::string>(content_);
    int font_size = (int)(dest.height * 0.5f);
    int text_width = MeasureText(text.c_str(), font_size);
    DrawText(text.c_str(), (int)(dest.x + (dest.width - text_width) / 2),
             (int)(dest.y + (dest.height - font_size) / 2), font_size, BLACK);
  } else if (std::holds_alternative<const Texture2D *>(content_)) {
    const Texture2D &image = *std::get<const Texture2D *>(content_);
    DrawTexturePro(image, {0, 0, (float)image.width, (float)image.height}, dest,
                   {0, 0}, 0.0f, WHITE);
  }
}
