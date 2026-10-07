#pragma once

#include <array>

#include "position.hpp"

namespace quarto {

// True if line is completely filled and its four pieces share a property.
bool line_is_win(Bitboard line, Bitboard occ,
                 const std::array<Bitboard, 4> &prop);

bool move_wins(Cell cell, Bitboard occ, const std::array<Bitboard, 4> &prop);

// Same check against a live Position - the usual call right after
// p.place_piece(cell).
bool move_wins(const Position &p, Cell cell);

Score eval(Position &pos);

} // namespace quarto
