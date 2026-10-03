// layout.hpp - Rect helpers for resolution-independent layout: split, inset,
// center, aspect-fit square.
#pragma once
#include "layout.hpp"
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

constexpr float NAVBAR_HEIGHT = 100;

constexpr float START_WIDTH = 500;
constexpr float START_HEIGHT = 200;
constexpr float START_X = (DESIGN_WIDTH - START_WIDTH) / 2.0f;
constexpr float START_Y = 200;

constexpr int POOL_COLS = 8;
constexpr int POOL_ROWS = 16 / POOL_COLS;
constexpr float POOL_TILE = 125;
constexpr float POOL_X = DESIGN_WIDTH / 2 - (int)(POOL_COLS / 2) * POOL_TILE;
constexpr float POOL_Y = 850 - (int)(POOL_ROWS / 2) * POOL_TILE;

constexpr float BOARD_TILE = 125;
constexpr float BOARD_X = DESIGN_WIDTH / 2 - 2 * BOARD_TILE;
constexpr float BOARD_Y = DESIGN_HEIGHT / 2 - 2 * BOARD_TILE - 125;
constexpr float BOARD_PADDING = 10;

constexpr float HAND_X = DESIGN_WIDTH / 2;
constexpr float HAND_Y = 200;
constexpr float HAND_SIZE = 200;

constexpr float PLAYER_WIDTH = 400;
constexpr float PLAYER_HEIGHT = DESIGN_HEIGHT - NAVBAR_HEIGHT;
constexpr float PLAYER_1_X = 0;
constexpr float PLAYER_2_X = DESIGN_WIDTH - PLAYER_WIDTH;
constexpr float PLAYER_Y = NAVBAR_HEIGHT;

constexpr float EVALBAR_MARGIN = 25;
constexpr float EVALBAR_WIDTH = 25;
constexpr float EVALBAR_1_X = PLAYER_1_X + PLAYER_WIDTH - 2.0f * EVALBAR_MARGIN;
constexpr float EVALBAR_2_X = PLAYER_2_X + EVALBAR_MARGIN;
constexpr float EVALBAR_HEIGHT = PLAYER_HEIGHT - 2 * EVALBAR_MARGIN;
constexpr float EVALBAR_Y = PLAYER_Y + EVALBAR_MARGIN;

constexpr float BUTTON_WIDTH = 50;
constexpr float BUTTON_HEIGHT = 50;

constexpr float PLAYER_DD_WIDTH = 250;
constexpr float PLAYER_DD_HEIGHT = 100;
constexpr float PLAYER_1_DD_X = DESIGN_WIDTH / 2.0f - PLAYER_DD_WIDTH;
constexpr float PLAYER_2_DD_X = DESIGN_WIDTH / 2.0f;
constexpr float PLAYER_DD_Y = 600;
