#include "ordering.hpp"
#include "tables.hpp"
#include "types.hpp"
#include <bit>

namespace quarto {

Cell best_cell(Bitboard &empty, std::array<Score, 16> &prior) {
  Score best = std::countr_zero(empty);
  for (Bitboard b = Bitboard(empty & (empty - 1)); b; b &= b - 1) {
    const int i = std::countr_zero(b);
    if (prior[i] < prior[best]) {
      best = i;
    }
  }
  empty &= Bitboard(~(Bitboard{1} << best));
  return best;
};

std::array<Score, 16> compute_prior(const Position &pos) {
  const auto &table = squares_win ? SQUARE_COMPLETES : LINE_COMPLETES;
  Piece hand = pos.in_hand();
  Bitboard occ = pos.occupied();
  Bitboard empty = pos.empty_cells();
  std::array<Bitboard, 4> same = pos.props();
  std::array<Score, 16> prior;
  prior.fill(0);
  for (int i = 0; i < 4; ++i) {
    Bitboard opp;
    if (hand & (Bitboard{1} << i)) {
      opp = ~same[i] & occ;
    } else {
      opp = same[i];
      same[i] = ~same[i] & occ;
    }
    for (Bitboard blocks = table[opp] & empty; blocks; blocks &= blocks - 1) {
      Cell c = std::countr_zero(blocks);
      prior[c] += 2;
    }
    for (Bitboard e = empty; e; e &= e - 1) {
      Cell c = std::countr_zero(e);
      prior[c] -=
          std::popcount(Bitboard(table[same[i] | (Bitboard{1} << c)] & empty));
    }
  }
  return prior;
};

} // namespace quarto
