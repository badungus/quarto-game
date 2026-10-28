// colors.hpp - Declares the UI colour palette used to tint the panel texture.
#pragma once

#include <raylib.h>

namespace colors {

// Tints for the white panel texture: normal, hover and armed buttons.
constexpr Color PANEL = {0x97, 0xBE, 0xC0, 0xFF};
constexpr Color PANEL_HOVER = {0x6C, 0xA1, 0xA3, 0xFF};
constexpr Color PANEL_ARMED = {0x42, 0x8A, 0x8E, 0xFF};

// Outline of the panel texture, also used for text.
constexpr Color OUTLINE = {0x1E, 0x1E, 0x1E, 0xFF};

} // namespace colors
