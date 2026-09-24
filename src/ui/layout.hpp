// layout.hpp - Rect helpers for resolution-independent layout: split, inset,
// center, aspect-fit square.
#pragma once
#include <algorithm>
#include <raylib.h>

constexpr float DESIGN_WIDTH = 1920.0f;
constexpr float DESIGN_HEIGHT = 1080.0f;

// Must be evaluated after InitWindow(), so it's a function, not a global.
inline float ui_scale() {
  return std::min(GetScreenWidth() / DESIGN_WIDTH,
                  GetScreenHeight() / DESIGN_HEIGHT);
}

constexpr float PIECEPOOLX = 1500;
constexpr float POOLBUTTONSIZE = 100;
constexpr float PIECEPOOLY = 600;
