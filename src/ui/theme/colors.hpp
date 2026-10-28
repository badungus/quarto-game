// colors.hpp - Declares the UI colour palette used to tint the panel texture.
// Dark values of scheme-content.json, mapped to the UI as in the
// quarto-ui-colors mockup.
#pragma once

#include <raylib.h>

// Tints for the white panel texture by button state, plus the colour of its
// text or icon.
struct ButtonColors {
  Color normal;
  Color hover;
  Color armed;
  Color content;
};

namespace colors {

// base on with percent% of on layered on top, the way hover and pressed
// states are drawn in the mockup.
constexpr Color layer(Color base, Color on, int percent) {
  auto mix = [&](unsigned char b, unsigned char o) {
    return (unsigned char)((b * (100 - percent) + o * percent + 50) / 100);
  };
  return {mix(base.r, on.r), mix(base.g, on.g), mix(base.b, on.b), 255};
}
constexpr int HOVER = 8;
constexpr int PRESSED = 14;

// c drawn with tint, as raylib does: each channel multiplied.
constexpr Color tinted(Color c, Color tint) {
  auto mul = [](unsigned char a, unsigned char b) {
    return (unsigned char)(a * b / 255);
  };
  return {mul(c.r, tint.r), mul(c.g, tint.g), mul(c.b, tint.b), 255};
}

// Surfaces, darkest first.
constexpr Color SURFACE_LOWEST = {0x0C, 0x0F, 0x0F, 0xFF};
constexpr Color SURFACE_HIGH = {0x28, 0x2A, 0x2B, 0xFF}; // navbar, panels
// Outline baked into panel.png, also drawn along the navbar and panels.
constexpr Color OUTLINE = {0x1E, 0x1E, 0x1E, 0xFF};
// In screen pixels: DrawTextureNPatch draws the texture's borders unscaled.
constexpr float OUTLINE_WIDTH = 5.0f;
constexpr Color ARENA = {0x30, 0x38, 0x3A, 0xFF}; // window background
constexpr Color ON_SURFACE = {0xE2, 0xE2, 0xE3, 0xFF};
constexpr Color ON_SURFACE_VARIANT = {0xC0, 0xC8, 0xC9, 0xFF};

constexpr Color PRIMARY = {0xA4, 0xCE, 0xD6, 0xFF};
constexpr Color ON_PRIMARY = {0x06, 0x36, 0x3D, 0xFF};
constexpr Color PRIMARY_CONTAINER = {0x77, 0xA0, 0xA8, 0xFF};
constexpr Color ON_PRIMARY_CONTAINER = {0x00, 0x09, 0x0C, 0xFF};
constexpr Color SECONDARY_CONTAINER = {0x3A, 0x49, 0x4C, 0xFF};
constexpr Color ON_SECONDARY_CONTAINER = {0xD3, 0xE3, 0xE6, 0xFF};
// Piece in hand, player to move, winning line.
constexpr Color TERTIARY = {0xD0, 0xC0, 0xE5, 0xFF};

// Evalbar: the light bar grows from the bottom over the dark track.
constexpr Color EVAL_LIGHT = PRIMARY;
constexpr Color EVAL_DARK = SURFACE_LOWEST; // same as the board background

constexpr Color CELL = {0x80, 0x9B, 0x9D, 0xFF}; // base04, brightened
constexpr Color SLOT = {0x4F, 0x5F, 0x61, 0xFF}; // base02
// Behind the pool: the outline of the slots as it shows on screen.
constexpr Color POOL_FRAME = tinted(OUTLINE, SLOT);

// Main actions: start, engine match.
constexpr ButtonColors PRIMARY_BUTTON = {
    PRIMARY, layer(PRIMARY, ON_PRIMARY, HOVER),
    layer(PRIMARY, ON_PRIMARY, PRESSED), ON_PRIMARY};
// Everything else: game controls, dropdowns, text fields, toggles, home.
constexpr ButtonColors SECONDARY_BUTTON = {
    SECONDARY_CONTAINER,
    layer(SECONDARY_CONTAINER, ON_SECONDARY_CONTAINER, HOVER),
    layer(SECONDARY_CONTAINER, ON_SECONDARY_CONTAINER, PRESSED),
    ON_SECONDARY_CONTAINER};
// Board cells and pool slots. Content is WHITE so pieces keep their colours.
// Cells darken on hover and press, since they are the brightest buttons.
constexpr ButtonColors CELL_BUTTON = {CELL, layer(CELL, SURFACE_LOWEST, HOVER),
                                      layer(CELL, SURFACE_LOWEST, PRESSED),
                                      WHITE};
constexpr ButtonColors SLOT_BUTTON = {SLOT, layer(SLOT, ON_SURFACE, HOVER),
                                      layer(SLOT, ON_SURFACE, PRESSED), WHITE};

} // namespace colors
