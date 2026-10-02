#pragma once

#include <array>

#include "types.hpp"

namespace quarto {

inline constexpr std::array<Bitboard, 10> LINES = {
    0x000F, 0x00F0, 0x0F00, 0xF000, // rows 0-3
    0x1111, 0x2222, 0x4444, 0x8888, // cols 0-3
    0x8421, 0x1248                  // diagonals
};

// The lines through one cell: 2 normally, 3 on a diagonal. Fixed capacity so
// the whole table can be constexpr - a std::vector cannot outlive constant
// evaluation.
struct CellLines {
  std::array<Bitboard, 3> lines{};
  int count = 0;

  constexpr const Bitboard *begin() const { return lines.data(); }
  constexpr const Bitboard *end() const { return lines.data() + count; }
};

constexpr std::array<CellLines, 16> makeCellLines() {
  std::array<CellLines, 16> result{};

  for (int cell = 0; cell < 16; cell++) {
    for (Bitboard line : LINES) {
      if ((line >> cell) & 1) {
        result[cell].lines[result[cell].count++] = line;
      }
    }
  }

  return result;
}

inline constexpr std::array<CellLines, 16> CELL_LINES = makeCellLines();
// every run of the engine uses the identical table
} // namespace quarto
