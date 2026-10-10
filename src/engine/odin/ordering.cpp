#include "ordering.hpp"
#include "types.hpp"
#include <bit>

namespace quarto {

Cell best_cell(Bitboard &empty, std::array<std::array<int, 16>, 16> &history,
               const Piece &hand) {
  int best = std::countr_zero(empty);
  for (Bitboard b = empty & (empty - 1); b; b &= b - 1) {
    int i = std::countr_zero(b);
    if (history[hand][i] > history[hand][best]) {
      best = i;
    }
  }
  empty &= ~(Bitboard{1} << best);
  return best;
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
