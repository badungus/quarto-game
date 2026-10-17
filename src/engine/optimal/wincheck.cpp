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

namespace {
// COMPLETES[m]: every cell that is the only cell of some line outside the
// cells m, so it completes a line whose other three cells are in m. One table
// for the lines, one for the lines and 2x2 squares. Built at startup: too much
// work for a constexpr.
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
const std::array<Bitboard, 1 << 16> LINE_COMPLETES = make_completes(LINES);
const std::array<Bitboard, 1 << 16> SQUARE_COMPLETES =
    make_completes(WIN_CONDITIONS);
} // namespace

Bitboard winning_pieces(Position &pos) {
  const auto &completes = squares_win ? SQUARE_COMPLETES : LINE_COMPLETES;
  Bitboard empty = pos.empty_cells();
  Bitboard wins = 0;
  // A line of three pieces sharing a property value, with its fourth cell
  // empty, is won by any piece with that value. The cells holding each value
  // are looked up for such a fourth cell.
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
