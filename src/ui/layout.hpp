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

constexpr int BORDER = 20;

constexpr float SCREEN_BORDER = 15;

constexpr int POOL_COLS = 8;
constexpr int POOL_ROWS = 16 / POOL_COLS;
constexpr float POOL_TILE = 100;
constexpr float POOL_X = 960 - (int)(POOL_COLS / 2) * POOL_TILE;
constexpr float POOL_Y = 900 - (int)(POOL_ROWS / 2) * POOL_TILE;

constexpr float BOARD_X = 960 - 2 * POOL_TILE;
constexpr float BOARD_Y = 550 - 2 * POOL_TILE;
constexpr float BOARD_TILE = 100;
constexpr float BOARD_PADDING = 10;

constexpr float HAND_X = 960;
constexpr float HAND_Y = 200;
constexpr float HAND_SIZE = 200;

constexpr float PLAYER_WIDTH = 400;
constexpr float PLAYER_HEIGHT = 1080 - 2 * SCREEN_BORDER;
constexpr float PLAYER_1_X = SCREEN_BORDER;
constexpr float PLAYER_2_X = 1920 - SCREEN_BORDER - PLAYER_WIDTH;
constexpr float PLAYER_Y = SCREEN_BORDER;

constexpr float BUTTON_WIDTH = 50;
constexpr float BUTTON_HEIGHT = 50;
