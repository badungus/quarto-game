#include "wincheck.hpp"
#include "tables.hpp"

namespace quarto {

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

bool move_wins(Cell cell, Bitboard occ, const std::array<Bitboard, 4> &prop) {
  const auto &cell_lines = squares_win ? CELL_SQUARE_LINES : CELL_LINES;
  for (Bitboard line : cell_lines[cell]) {
    if (line_is_win(line, occ, prop))
      return true;
  }
  return false;
}

bool move_wins(const Position &p, Cell cell) {
  return move_wins(cell, p.occupied(), p.props());
}

Bitboard winning_pieces(Position &pos) {
  const auto &completes = squares_win ? SQUARE_COMPLETES : LINE_COMPLETES;
  Bitboard empty = pos.empty_cells();
  Bitboard wins = 0;
  for (int i = 0; i < 4; ++i) {
    Bitboard ones = pos.props()[i];
    Bitboard zeros = Bitboard(pos.occupied() & ~ones);
    if (completes[ones] & empty)
      wins |= HAS_PROP[i];
    if (completes[zeros] & empty)
      wins |= ~HAS_PROP[i];
  }
  return wins;
}

} // namespace quarto
