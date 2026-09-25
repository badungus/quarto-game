// input.hpp - Declares Input: the mouse state for one frame, in design space.
#pragma once
#include <raylib.h>

struct Input {
  Vector2 mouse; // in 1920x1080 design space
  bool pressed;  // left button went down this frame
  bool released;
};
Input read_input();
