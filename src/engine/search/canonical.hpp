#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "position.hpp"
#include "types.hpp"

namespace quarto {

// the four permutations for regular quarto. 2x2 squares only uses {0,1,2,3}
inline constexpr std::array<std::array<int, 4>, 4> AXIS_PERMS = {{
    {0, 1, 2, 3},
    {0, 2, 1, 3},
    {1, 0, 3, 2},
    {1, 3, 0, 2},
}};

constexpr int SYM_COUNT = 32;

// apply rotation around 180 degrees, mirror and transpose. These correspond to
// the dihedral group for each of the permutations of AXIS_PERMS.
constexpr std::array<std::array<Cell, 16>, SYM_COUNT> make_syms() {
  std::array<std::array<Cell, 16>, SYM_COUNT> syms{};
  int n = 0;
  for (const auto &sigma : AXIS_PERMS) {
    for (int rot180 = 0; rot180 < 2; ++rot180) {
      for (int mirror = 0; mirror < 2; ++mirror) {
        for (int transpose = 0; transpose < 2; ++transpose) {
          for (int cell = 0; cell < 16; ++cell) {
            int r = sigma[cell / 4];
            int c = sigma[cell % 4];
            if (rot180) {
              r = 3 - r;
              c = 3 - c;
            }
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
  }
  return syms;
}

inline constexpr auto SYMS = make_syms();

// Bitboard for the symmetries of the 2x2 squares mode.
inline constexpr std::uint32_t SQUARE_SYMS = 0x000000FFu;

// INV_SYMS[s] undoes SYMS[s].
constexpr std::array<std::array<Cell, 16>, SYM_COUNT> make_inv_syms() {
  std::array<std::array<Cell, 16>, SYM_COUNT> inv{};
  for (int s = 0; s < SYM_COUNT; ++s)
    for (int cell = 0; cell < 16; ++cell)
      inv[s][SYMS[s][cell]] = Cell(cell);
  return inv;
}

inline constexpr auto INV_SYMS = make_inv_syms();

struct Canon {
  std::uint64_t cols;
  Bitboard occ;                      // occupancy after `sym`
  bool has_hand;                     // in canonical form the hand is piece 0
  std::uint8_t sym;                  // board symmetry, index into SYMS
  std::array<std::uint8_t, 4> order; // canonical property j = original order[j]
  std::uint8_t flip;                 // bit i: original property i is inverted
};

Canon canonicalize(const Position &pos);

// Real frame to canonical frame and back
Move to_canonical(Move m, const Canon &c);
Move from_canonical(Move m, const Canon &c);

} // namespace quarto
