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

constexpr std::array<CellLines, 16> make_cell_lines() {
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

inline constexpr std::array<CellLines, 16> CELL_LINES = make_cell_lines();

// splitmix64: the standard seeding PRNG. One multiply-xorshift finalizer over a
// counter, which is enough here - the TT only needs its index well spread, not
// cryptographic.
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

inline constexpr std::array<Bitboard, 4> HAS_PROP = pieces_with_prop();

} // namespace quarto
