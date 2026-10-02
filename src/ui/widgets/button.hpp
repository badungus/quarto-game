// button.hpp - Declares the immediate-mode Button widget (normal, hover).
#pragma once
#include "input.hpp"
#include <raylib.h>
#include <string>
#include <variant>

using ButtonContent =
    std::variant<std::monostate, std::string, const Texture2D *>;

class Button {
private:
  Rectangle bounds_; // top-left and size, in design space
  bool hover_ = false;
  bool enabled_ = true;
  bool armed_ = false;
  // References, not copies, so textures loaded later are still seen.
  const Texture2D &background_;
  const Texture2D &hover_image_;
  const Texture2D &armed_image_;
  ButtonContent content_;

public:
  Button(Rectangle bounds, const Texture2D &background,
         const Texture2D &hover_image, const Texture2D &armed_image,
         ButtonContent content);
  // Updates hover state; returns true if the button was clicked this frame.
  bool update(const Input &in);
  void draw() const;
  void set_enabled(bool enabled) { enabled_ = enabled; }
  void set_content(ButtonContent content) { content_ = content; }
};
