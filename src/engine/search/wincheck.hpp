#pragma once

#include <array>

#include "position.hpp"

namespace quarto {

// True if line is completely filled and its four pieces share a property.
bool line_is_win(Bitboard line, Bitboard occ,
                 const std::array<Bitboard, 4> &prop);

bool move_wins(Cell cell, Bitboard occ, const std::array<Bitboard, 4> &prop);

bool move_wins(const Position &p, Cell cell);

// True if the piece in hand completes a winning line on some empty cell.
bool hand_wins(Position &pos);

// Score for the side to move winning with the piece in hand
Score win_score(const Position &pos);

Bitboard winning_pieces(Position &pos);

Score eval(Position &pos);

} // namespace quarto
