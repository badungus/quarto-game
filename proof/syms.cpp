// Compile-time proof that SYMS is exactly the symmetry group of the board,
// and SQUARE_SYMS exactly the symmetry group when 2x2 squares also win.
// Nothing runs: if this file compiles, the claims hold.
//
// A symmetry is a permutation of the 16 cells that maps the win lines onto
// win lines. There are 16! permutations, too many to try, so the proof has
// four steps:
//
//   1. Lemma: every symmetry sends rows/columns to rows/columns. Two lines are
//      disjoint exactly when both are rows, both are columns, or both are
//      diagonals. A symmetry is a bijection, so it keeps disjointness, and so
//      maps each of these groups onto a group of the same size: the diagonals
//      onto the diagonals, and the rows onto either the rows or the columns
//      (the columns then onto the other). A cell is the only cell its row and
//      column share, so the symmetry is fixed by a row permutation, a column
//      permutation and whether it transposes. lines_partition() checks the
//      disjointness fact the argument rests on.
//   2. So it is enough to try those 4! * 4! * 2 = 1152 maps. syms_complete()
//      tries them all: exactly 32 keep the lines, and each is in SYMS. SYMS
//      has 32 entries, so it is precisely that set, without duplicates.
//   3. With squares, a symmetry must keep all 19 win conditions. Lemma: it
//      then keeps the 10 lines on their own, so by steps 1-2 it is in SYMS.
//      A symmetry keeps disjointness, so a win condition and its image are
//      disjoint from equally many win conditions. That count is 3, 6 or 9
//      for a line and 4, 7 or 10 for a square (lines_apart() checks this),
//      so no line can become a square.
//   4. squares_complete() checks that the symmetries in SYMS that keep the
//      19 win conditions are exactly the ones in SQUARE_SYMS. With step 3,
//      SQUARE_SYMS is the whole symmetry group of the squares mode.
//
// The same holds for odin, whose SYMS is identical.

#include "canonical.hpp"
#include "tables.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace quarto {
namespace {

// The image of a set of cells under a cell map.
constexpr Bitboard image(Bitboard set, const std::array<Cell, 16> &map) {
  Bitboard m = 0;
  for (int i = 0; i < 16; ++i)
    if ((set >> i) & 1)
      m |= Bitboard(1u << map[i]);
  return m;
}

template <std::size_t N>
constexpr bool keeps(const std::array<Bitboard, N> &family,
                     const std::array<Cell, 16> &map) {
  for (Bitboard s : family)
    if (std::find(family.begin(), family.end(), image(s, map)) ==
        family.end())
      return false;
  return true;
}

// Step 1: lines are disjoint exactly within rows (LINES[0..3]), within
// columns (LINES[4..7]) and within diagonals (LINES[8..9]).
constexpr bool lines_partition() {
  auto group = [](int i) { return i < 4 ? 0 : i < 8 ? 1 : 2; };
  for (int i = 0; i < 10; ++i)
    for (int j = 0; j < 10; ++j)
      if (i != j && ((LINES[i] & LINES[j]) == 0) != (group(i) == group(j)))
        return false;
  return true;
}

// Step 2: every row/column permutation with optional transpose that keeps
// the lines is in SYMS, and there are SYM_COUNT of them.
constexpr bool syms_complete() {
  int count = 0;
  std::array<int, 4> p{0, 1, 2, 3};
  do {
    std::array<int, 4> q{0, 1, 2, 3};
    do {
      for (int t = 0; t < 2; ++t) {
        std::array<Cell, 16> map{};
        for (int cell = 0; cell < 16; ++cell) {
          int r = p[cell / 4], c = q[cell % 4];
          if (t)
            std::swap(r, c);
          map[cell] = Cell(r * 4 + c);
        }
        if (!keeps(LINES, map))
          continue;
        ++count;
        if (std::find(SYMS.begin(), SYMS.end(), map) == SYMS.end())
          return false; // a symmetry SYMS is missing
      }
    } while (std::next_permutation(q.begin(), q.end()));
  } while (std::next_permutation(p.begin(), p.end()));
  return count == SYM_COUNT;
}

// Step 3: every line is disjoint from a different number of win conditions
// than every square. WIN_CONDITIONS starts with the 10 LINES.
constexpr bool lines_apart() {
  auto disjoint_count = [](Bitboard s) {
    int n = 0;
    for (Bitboard t : WIN_CONDITIONS)
      if ((s & t) == 0)
        ++n;
    return n;
  };
  for (int i = 0; i < 10; ++i) {
    if (WIN_CONDITIONS[i] != LINES[i])
      return false;
    for (int j = 10; j < 19; ++j)
      if (disjoint_count(WIN_CONDITIONS[i]) ==
          disjoint_count(WIN_CONDITIONS[j]))
        return false;
  }
  return true;
}

// Step 4: SYMS[s] keeps every win condition, squares included, exactly when
// bit s of SQUARE_SYMS is set.
constexpr bool squares_complete() {
  for (int s = 0; s < SYM_COUNT; ++s)
    if (keeps(WIN_CONDITIONS, SYMS[s]) != (((SQUARE_SYMS >> s) & 1) != 0))
      return false;
  return true;
}

static_assert(lines_partition());
static_assert(syms_complete());
static_assert(lines_apart());
static_assert(squares_complete());

} // namespace
} // namespace quarto
