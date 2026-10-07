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

// True if the piece in hand completes a winning line on some empty cell.
bool hand_wins(Position &pos);

// Score for the side to move winning with the piece in hand: WIN minus the
// number of pieces on the board after the winning placement. It depends only on
// the position, never on the search depth, so it can be stored in a TT as is,
// and a faster win still scores higher.
Score win_score(const Position &pos);

// Static score for the side to move, in a position where the piece in hand
// cannot win (negamax checks hand_wins first): -(win_score(pos) - 1) if every
// placement + give lets the opponent win at once (with one more piece on the
// board), else 0.
Score eval(Position &pos);

Bitboard winning_pieces(Position &pos);

} // namespace quarto
