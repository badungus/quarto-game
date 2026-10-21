#pragma once

#include <array>
#include <bit>
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

// True when 2x2 squares win too
inline bool squares_win = false;

struct CellLines {
  std::array<Bitboard, 7> lines{};
  int count = 0;

  constexpr const Bitboard *begin() const { return lines.data(); }
  constexpr const Bitboard *end() const { return lines.data() + count; }
};

template <std::size_t N>
constexpr std::array<CellLines, 16>
make_cell_lines(const std::array<Bitboard, N> &lines) {
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

inline constexpr std::array<CellLines, 16> CELL_LINES = make_cell_lines(LINES);
// The same with the 2x2 squares, for when squares win.
inline constexpr std::array<CellLines, 16> CELL_SQUARE_LINES =
    make_cell_lines(WIN_CONDITIONS);

// splitmix64 for good distribution
constexpr Key splitmix64(Key &state) {
  state += 0x9E3779B97F4A7C15ULL;
  Key z = state;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

constexpr std::array<Bitboard, 4> pieces_with_prop() {
  std::array<Bitboard, 4> has;
  has.fill(0);
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 16; ++j) {
      if ((j >> i) & 1) {
        has[i] |= Bitboard{1} << j;
      }
    }
  }
  return has;
};

// HAS_PROP[i]: the pieces with property i set.
inline constexpr std::array<Bitboard, 4> HAS_PROP = pieces_with_prop();

// precompute the missing pieces for all possibilities to get 4 in a win
// condition
template <std::size_t N>
std::array<Bitboard, 1 << 16>
make_completes(const std::array<Bitboard, N> &lines) {
  std::array<Bitboard, 1 << 16> t{};
  for (unsigned m = 0; m < (1u << 16); ++m)
    for (Bitboard line : lines) {
      Bitboard missing = Bitboard(line & ~m);
      if (std::popcount(missing) == 1)
        t[m] |= missing;
    }
  return t;
}
inline const std::array<Bitboard, 1 << 16> LINE_COMPLETES =
    make_completes(LINES);
inline const std::array<Bitboard, 1 << 16> SQUARE_COMPLETES =
    make_completes(WIN_CONDITIONS);
} // namespace quarto
