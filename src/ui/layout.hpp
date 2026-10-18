// layout.hpp - Rect helpers for resolution-independent layout: split, inset,
// center, aspect-fit square.
#pragma once
#include "match_config.hpp"
#include <algorithm>
#include <raylib.h>

// Reference size for the scale only; positions use canvas_width/height().
constexpr float DESIGN_WIDTH = 1920.0f;
constexpr float DESIGN_HEIGHT = 1080.0f;

// Must be evaluated after InitWindow(), so it's a function, not a global.
inline float ui_scale() {
  return std::min(GetScreenWidth() / DESIGN_WIDTH,
                  GetScreenHeight() / DESIGN_HEIGHT);
}

// The screen in design units: DESIGN_WIDTH x DESIGN_HEIGHT on a 16:9 screen,
// wider or taller on other aspect ratios. Positions anchored to the right or
// bottom edge are functions of these, so they too need InitWindow() first.
inline float canvas_width() { return GetScreenWidth() / ui_scale(); }
inline float canvas_height() { return GetScreenHeight() / ui_scale(); }

constexpr int BORDER = 20;
constexpr float UI_SPACING = 25;

constexpr float NAVBAR_HEIGHT = 100;

constexpr float START_WIDTH = 500;
constexpr float START_HEIGHT = 150;
inline float START_X() { return (canvas_width() - START_WIDTH) / 2.0f; }
constexpr float START_Y = 125;

constexpr int POOL_COLS = 8;
constexpr int POOL_ROWS = 16 / POOL_COLS;
constexpr float POOL_TILE = 125;
inline float POOL_X() {
  return canvas_width() / 2 - (int)(POOL_COLS / 2) * POOL_TILE;
}
constexpr float POOL_Y = 850 - (int)(POOL_ROWS / 2) * POOL_TILE;

constexpr float BOARD_TILE = 125;
inline float BOARD_X() { return canvas_width() / 2 - 2 * BOARD_TILE; }
// Fixed like the hand and pool around it, so extra height goes below the pool.
constexpr float BOARD_Y = DESIGN_HEIGHT / 2 - 2 * BOARD_TILE - 125;
constexpr float BOARD_PADDING = 10;

inline float HAND_X() { return canvas_width() / 2; }
constexpr float HAND_Y = 200;
constexpr float HAND_SIZE = 200;

constexpr float PLAYER_WIDTH = 400;
inline float PLAYER_HEIGHT() { return canvas_height() - NAVBAR_HEIGHT; }
constexpr float PLAYER_1_X = 0;
inline float PLAYER_2_X() { return canvas_width() - PLAYER_WIDTH; }
constexpr float PLAYER_Y = NAVBAR_HEIGHT;

constexpr float PLAYER_NAME_WIDTH = PLAYER_WIDTH - 2 * UI_SPACING;
constexpr float PLAYER_1_NAME_X = PLAYER_1_X + UI_SPACING;
inline float PLAYER_2_NAME_X() { return PLAYER_2_X() + UI_SPACING; }
constexpr float PLAYER_NAME_Y = PLAYER_Y + UI_SPACING;
constexpr float PLAYER_NAME_HEIGHT = 100;

constexpr float EVALBAR_WIDTH = 25;
constexpr float EVALBAR_1_X =
    PLAYER_1_X + PLAYER_WIDTH - UI_SPACING - EVALBAR_WIDTH;
inline float EVALBAR_2_X() { return PLAYER_2_X() + UI_SPACING; }
inline float EVALBAR_HEIGHT() {
  return PLAYER_HEIGHT() - 3 * UI_SPACING - PLAYER_NAME_HEIGHT;
}
constexpr float EVALBAR_Y = PLAYER_Y + 2 * UI_SPACING + PLAYER_NAME_HEIGHT;

constexpr float BUTTON_WIDTH = 50;
constexpr float BUTTON_HEIGHT = 50;
constexpr int BUTTON_COUNT = 6;

constexpr float PLAYER_DD_WIDTH = 250;
constexpr float PLAYER_DD_HEIGHT = 100;
inline float PLAYER_1_DD_X() { return canvas_width() / 2.0f - PLAYER_DD_WIDTH; }
inline float PLAYER_2_DD_X() { return canvas_width() / 2.0f; }
constexpr float PLAYER_DD_Y = 600;

// Engine settings stack upwards from just above the player dropdowns.
constexpr float SETTING_HEIGHT = 75;
constexpr float SETTING_BOTTOM_Y = PLAYER_DD_Y - UI_SPACING;

// Main menu, top to bottom: Start, engine settings, player dropdowns, Engine
// Match. The dropdowns open downwards over their own row, so they get room for
// every entry above the Engine Match button.
constexpr float MENU_MATCH_HEIGHT = 100;
inline float MENU_MATCH_Y() {
  return canvas_height() - UI_SPACING - MENU_MATCH_HEIGHT;
}
constexpr float MENU_DD_HEIGHT = SETTING_HEIGHT;
inline float MENU_DD_Y() {
  return MENU_MATCH_Y() - UI_SPACING - PLAYERS.size() * MENU_DD_HEIGHT;
}
inline float MENU_SETTING_BOTTOM_Y() { return MENU_DD_Y() - UI_SPACING; }
// The rules toggle sits under the closed dropdowns, in the space their open
// lists cover, so it is hidden while either list is open.
inline float MENU_RULES_Y() {
  return MENU_DD_Y() + MENU_DD_HEIGHT + UI_SPACING;
}

// Engine match screen: controls in two columns on the left, the match output
// in a panel filling the rest. Settings and dropdowns keep the main menu's
// heights.
constexpr float MATCH_COLUMN_1_X = UI_SPACING;
constexpr float MATCH_COLUMN_2_X = MATCH_COLUMN_1_X + PLAYER_DD_WIDTH;
constexpr float MATCH_START_Y = NAVBAR_HEIGHT + UI_SPACING;
constexpr float MATCH_START_HEIGHT = 100;
constexpr float MATCH_FIELD_Y = MATCH_START_Y + MATCH_START_HEIGHT + UI_SPACING;
// As in the main menu, the rules toggle sits under the closed dropdowns and is
// hidden while either list is open.
constexpr float MATCH_RULES_Y = PLAYER_DD_Y + PLAYER_DD_HEIGHT + UI_SPACING;
constexpr float MATCH_OUTPUT_X =
    MATCH_COLUMN_2_X + PLAYER_DD_WIDTH + UI_SPACING;
constexpr float MATCH_OUTPUT_Y = MATCH_START_Y;
inline float MATCH_OUTPUT_WIDTH() {
  return canvas_width() - MATCH_OUTPUT_X - UI_SPACING;
}
inline float MATCH_OUTPUT_HEIGHT() {
  return canvas_height() - MATCH_OUTPUT_Y - UI_SPACING;
}
constexpr float MATCH_OUTPUT_PADDING = 30;
constexpr float MATCH_OUTPUT_FONT = 22;
constexpr float MATCH_OUTPUT_LINE = 30;

constexpr float ENGINE_OUTPUT_PLAYER_1_X = PLAYER_1_NAME_X;
inline float ENGINE_OUTPUT_PLAYER_2_X() {
  return PLAYER_2_NAME_X() + EVALBAR_WIDTH + UI_SPACING;
}
constexpr float ENGINE_OUTPUT_Y =
    PLAYER_NAME_Y + PLAYER_NAME_HEIGHT + UI_SPACING;
constexpr float ENGINE_OUTPUT_SIZE = 12;
constexpr float ENGINE_OUTPUT_LINE = 0.7f * UI_SPACING;
// Up to the spacing before player one's evalbar; player two's column is just
// as wide.
constexpr float ENGINE_OUTPUT_WIDTH =
    EVALBAR_1_X - UI_SPACING - ENGINE_OUTPUT_PLAYER_1_X;
// Output stops where the evalbars end, inside the player panels.
inline float ENGINE_OUTPUT_BOTTOM() { return EVALBAR_Y + EVALBAR_HEIGHT(); }

constexpr float HOME_BUTTON_WIDTH = 300;
constexpr float HOME_BUTTON_HEIGHT = 0.8f * NAVBAR_HEIGHT;
inline float HOME_BUTTON_X() {
  return (canvas_width() - HOME_BUTTON_WIDTH) / 2;
}
constexpr float HOME_BUTTON_Y = (NAVBAR_HEIGHT - HOME_BUTTON_HEIGHT) / 2;
