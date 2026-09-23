#include "wincheck.hpp"

namespace quarto {

bool line_is_win(Bitboard line, Bitboard occ, const std::array<Bitboard, 4>& prop)
{
    if ((line & occ) != line) return false;   // line not full yet

    for (Bitboard p : prop) {
        const Bitboard all_1 = Bitboard(p & line);
        const Bitboard all_0 = Bitboard(~p & line & occ);
        if (all_1 == line || all_0 == line) return true;
    }
    return false;                             // full line, no shared property
}

bool move_wins(Cell cell, Bitboard occ, const std::array<Bitboard, 4>& prop)
{
    for (Bitboard line : CELL_LINES[cell]) {
        if (line_is_win(line, occ, prop)) return true;
    }
    return false;
}

bool move_wins(const Position& p, Cell cell)
{
    return move_wins(cell, p.occupied(), p.props());
}

}  // namespace quarto
