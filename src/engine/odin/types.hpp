#pragma once

#include <cstdint>

namespace quarto {

using Bitboard = std::uint16_t;   // one bit per cell, 0..15
using Key      = std::uint64_t;   // Zobrist hash
using Piece    = std::int8_t;     // 0..15 (4 property bits), -1 = none
using Cell     = std::int8_t;     // 0..15 board index (row * 4 + col), -1 = none
using Score    = std::int16_t;

struct Move { Cell cell; Piece give; };   // a Quarto move is ALWAYS this pair
struct Undo { Cell cell; Piece hand; };

constexpr Score WIN = 10000;
constexpr Score INF = 32000;

}  // namespace quarto
