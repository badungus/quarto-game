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
  const Texture2D &panel_; // white, tinted by state
  ButtonContent content_;
  float text_scale_; // multiplies the default text size

public:
  Button(Rectangle bounds, const Texture2D &panel, ButtonContent content,
         float text_scale = 1.0f);
  // Updates hover state; returns true if the button was clicked this frame.
  bool update(const Input &in);
  void draw() const;
  void set_enabled(bool enabled) { enabled_ = enabled; }
  void set_content(ButtonContent content) { content_ = content; }
};
