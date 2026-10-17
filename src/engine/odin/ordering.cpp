#include "ordering.hpp"
#include "tables.hpp"
#include "types.hpp"
#include <bit>
#include <cstdint>

namespace quarto {

Cell best_cell(Bitboard &empty, std::array<Score, 16> &prior) {
  Score best = INT16_MAX;
  Cell best_cell = -1;
  for (int i = 0; i < 16; ++i) {
    if (prior[i] < best && empty & (Bitboard{1} << i)) {
      best_cell = i;
      best = prior[i];
    }
  }
  empty &= ~(Bitboard{1} << best_cell);
  return best_cell;
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

Piece best_give(Bitboard &avail, std::array<int, 16> &history) {

  int best = std::countr_zero(avail);
  for (Bitboard b = avail & (avail - 1); b; b &= b - 1) {
    int i = std::countr_zero(b);
    if (history[i] > history[best]) {
      best = i;
    }
  }
  avail &= ~(Bitboard{1} << best);
  return best;
};

} // namespace quarto
