#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "position.hpp"
#include "types.hpp"

namespace quarto {

// Two positions are equivalent if one of 32 board symmetries combined with one
// of 384 piece relabelings (reorder the four properties, invert any of them)
// turns one into the other. Equivalent positions have the same score at the
// same depth, so they can share a TT entry.

// Permutations of a row/column index that commute with i -> 3 - i. Applied to
// rows and columns together they keep both diagonals diagonals. {0, 2, 1, 3}
// is the "mid flip", {1, 0, 3, 2} "inside out".
inline constexpr std::array<std::array<int, 4>, 8> AXIS_PERMS = {{
    {0, 1, 2, 3},
    {0, 2, 1, 3},
    {1, 0, 3, 2},
    {1, 3, 0, 2},
    {2, 0, 3, 1},
    {2, 3, 0, 1},
    {3, 1, 2, 0},
    {3, 2, 1, 0},
}};

constexpr int SYM_COUNT = 32;

// SYMS[s][cell] is where symmetry s sends cell. SYMS[0] is the identity. Each
// is an axis permutation on rows and columns, optionally mirrored left-right,
// optionally transposed: 8 * 2 * 2 = 32, and every one maps LINES onto LINES.
constexpr std::array<std::array<Cell, 16>, SYM_COUNT> make_syms() {
  std::array<std::array<Cell, 16>, SYM_COUNT> syms{};
  int n = 0;
  for (const auto &sigma : AXIS_PERMS) {
    for (int mirror = 0; mirror < 2; ++mirror) {
      for (int transpose = 0; transpose < 2; ++transpose) {
        for (int cell = 0; cell < 16; ++cell) {
          int r = sigma[cell / 4];
          int c = sigma[cell % 4];
          if (mirror)
            c = 3 - c;
          if (transpose)
            std::swap(r, c);
          syms[n][cell] = Cell(r * 4 + c);
        }
        ++n;
      }
    }
  }
  return syms;
}

inline constexpr auto SYMS = make_syms();

// INV_SYMS[s] undoes SYMS[s].
constexpr std::array<std::array<Cell, 16>, SYM_COUNT> make_inv_syms() {
  std::array<std::array<Cell, 16>, SYM_COUNT> inv{};
  for (int s = 0; s < SYM_COUNT; ++s)
    for (int cell = 0; cell < 16; ++cell)
      inv[s][SYMS[s][cell]] = Cell(cell);
  return inv;
}

inline constexpr auto INV_SYMS = make_inv_syms();

// A position under one board symmetry with the piece relabeling normalised,
// and the transformation that got there. canonicalize returns the one that
// packs smallest: the representative of all equivalent positions, so `sym`
// is often not 0.
struct Canon {
  // The four property columns after the relabeling, sorted ascending, 16 bits
  // each: column j is bits 16j..16j+15.
  std::uint64_t cols;
  Bitboard occ;                      // occupancy after `sym`
  bool has_hand;                     // in canonical form the hand is piece 0
  std::uint8_t sym;                  // board symmetry, index into SYMS
  std::array<std::uint8_t, 4> order; // canonical property j = original order[j]
  std::uint8_t flip;                 // bit i: original property i is inverted
};

Canon canonicalize(const Position &pos);

// Real frame -> canonical frame and back. -1 cells and pieces pass through.
Move to_canonical(Move m, const Canon &c);
Move from_canonical(Move m, const Canon &c);

} // namespace quarto
