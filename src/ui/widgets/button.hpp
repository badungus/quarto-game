// button.hpp - Declares the immediate-mode Button widget (normal, hover).
#pragma once
#include "../input.hpp"
#include <raylib.h>

class Button {
private:
  Rectangle bounds_; // top-left and size, in design space
  bool hover_ = false;
  // References, not copies, so textures loaded later are still seen.
  const Texture2D &background_;
  const Texture2D &hover_image_;

public:
  Button(Rectangle bounds, const Texture2D &background,
         const Texture2D &hover_image);
  // Updates hover state; returns true if the button was clicked this frame.
  bool update(const Input &in);
  void draw() const;
};
