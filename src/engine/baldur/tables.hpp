#pragma once

#include <array>
#include <cstddef>

#include "types.hpp"

namespace quarto {

inline constexpr std::array<Bitboard, 10> LINES = {
    0x000F, 0x00F0, 0x0F00, 0xF000, // rows 0-3
    0x1111, 0x2222, 0x4444, 0x8888, // cols 0-3
    0x8421, 0x1248                  // diagonals
};

// LINES plus the nine 2x2 squares: what wins when squares win too.
inline constexpr std::array<Bitboard, 19> WIN_CONDITIONS = {
    0x000F, 0x00F0, 0x0F00, 0xF000,         // rows 0-3
    0x1111, 0x2222, 0x4444, 0x8888,         // cols 0-3
    0x8421, 0x1248,                         // diagonals
    0x0033, 0x0066, 0x00CC, 0x0330, 0x0660, // 2x2 squares
    0x0CC0, 0x3300, 0x6600, 0xCC00};        // 2x2 squares

// True when 2x2 squares win too ("go ... squares"). Engine::go sets it before
// each search starts, and it stays fixed while the search runs.
inline bool squares_win = false;

// The lines through one cell: 2 normally, 3 on a diagonal, and up to 4 more
// 2x2 squares when squares win. Fixed capacity so the whole table can be
// constexpr - a std::vector cannot outlive constant evaluation.
struct CellLines {
  std::array<Bitboard, 7> lines{};
  int count = 0;

  constexpr const Bitboard *begin() const { return lines.data(); }
  constexpr const Bitboard *end() const { return lines.data() + count; }
};

template <std::size_t N>
constexpr std::array<CellLines, 16>
makeCellLines(const std::array<Bitboard, N> &lines) {
  std::array<CellLines, 16> result{};

  for (int cell = 0; cell < 16; cell++) {
    for (Bitboard line : lines) {
      if ((line >> cell) & 1) {
        result[cell].lines[result[cell].count++] = line;
      }
    }
  }

  return result;
}

inline constexpr std::array<CellLines, 16> CELL_LINES = makeCellLines(LINES);
// The same with the 2x2 squares, for when squares win.
inline constexpr std::array<CellLines, 16> CELL_SQUARE_LINES =
    makeCellLines(WIN_CONDITIONS);
// every run of the engine uses the identical table
} // namespace quarto
