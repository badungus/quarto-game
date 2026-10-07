#include "wincheck.hpp"
#include "tables.hpp"
#include <bit>

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
  for (Bitboard line : CELL_LINES[cell]) {
    if (line_is_win(line, occ, prop))
      return true;
  }
  return false;
}

bool move_wins(const Position &p, Cell cell) {
  return move_wins(cell, p.occupied(), p.props());
}

bool hand_wins(Position &pos) {
  Bitboard empty = pos.empty_cells();
  while (empty) {
    Cell cell = std::countr_zero(empty);
    pos.place_piece(cell);
    bool win = move_wins(pos, cell);
    pos.undo_place_piece();
    if (win)
      return true;
    empty &= (empty - 1);
  }
  return false;
}

Score win_score(const Position &pos) {
  return Score(WIN - (std::popcount(pos.occupied()) + 1));
}

Score eval(Position &pos) {
  // Lost if every placement + give lets the opponent win at once. That win
  // puts one more piece on the board than a win from here would.
  Bitboard empty = pos.empty_cells();
  while (empty) {
    Cell cell = std::countr_zero(empty);
    pos.place_piece(cell);
    Bitboard avail = pos.available();
    // No piece left to give means the board is full: a draw, not a loss.
    bool safe = avail == 0 || (avail & ~winning_pieces(pos)) != 0;
    pos.undo_place_piece();
    if (safe)
      return 0;
    empty &= (empty - 1);
  }
  return Score(-(win_score(pos) - 1));
}

Bitboard winning_pieces(Position &pos) {
  Bitboard occ = pos.occupied();
  Bitboard wins = 0;
  for (Bitboard line : LINES) {
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
