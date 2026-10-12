#include "wincheck.hpp"
#include "tables.hpp"
#include <bit>
#include <span>

namespace quarto {

namespace {
std::span<const Bitboard> win_lines() {
  if (squares_win)
    return WIN_CONDITIONS;
  return LINES;
}
} // namespace

bool line_is_win(Bitboard line, Bitboard occ,
                 const std::array<Bitboard, 4> &prop) {
  if ((line & occ) != line)
    return false; // line not full yet

  for (Bitboard p : prop) {
    const Bitboard all_1 = Bitboard(p & line);
    const Bitboard all_0 = Bitboard(~p & line & occ);
    if (all_1 == line || all_0 == line)
      return true;
  }
  return false; // full line, no shared property
}

bool move_wins(const Position &p, Cell cell) {

  for (Bitboard line : win_lines()) {
    if (((line >> cell) & 1) && line_is_win(line, p.occupied(), p.props()))
      return true;
  }
  return false;
}

Bitboard winning_pieces(Position &pos) {
  Bitboard occ = pos.occupied();
  Bitboard wins = 0;
  for (Bitboard line : win_lines()) {
    Bitboard filled = line & occ;
    if (std::popcount(filled) != 3)
      continue;
    for (int i = 0; i < 4; ++i) {
      Bitboard with = pos.props()[i] & filled;
      if (with == filled)
        wins |= HAS_PROP[i];
      else if (with == 0)
        wins |= ~HAS_PROP[i];
    }
  }
  return wins;
}

} // namespace quarto
