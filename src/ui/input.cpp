// input.cpp - Implements gathering the input snapshot from raylib once per
// frame.
#include "input.hpp"
#include "layout.hpp"
#include <raylib.h>

Input read_input() {
  float s = ui_scale();
  Vector2 m = GetMousePosition();
  return {{m.x / s, m.y / s},
          IsMouseButtonPressed(MOUSE_BUTTON_LEFT),
          IsMouseButtonReleased(MOUSE_BUTTON_LEFT),
          IsMouseButtonDown(MOUSE_BUTTON_LEFT)};
}
