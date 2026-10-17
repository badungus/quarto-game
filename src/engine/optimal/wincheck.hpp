#pragma once

#include <array>

#include "position.hpp"

namespace quarto {

// True if line is completely filled and its four pieces share a property.
bool line_is_win(Bitboard line, Bitboard occ,
                 const std::array<Bitboard, 4> &prop);
// Same check against a live Position - the usual call right after
// p.place_piece(cell).
bool move_wins(const Position &p, Cell cell);

// The pieces that would complete a line if placed now: one line of three
// sharing a property is enough.
Bitboard winning_pieces(Position &pos);

// A score per cell for placing the piece in hand there, lowest first
// (indjev99's prior): +2 per property on which the cell blocks a line of three
// with the value the hand lacks, -1 per new empty cell that would complete a
// line of three with the hand's value. Cells that are not empty score 0.
std::array<int, 16> cell_priors(const Position &pos);

} // namespace quarto
