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
// the position, never on the search depth, so a faster win still scores
// higher. Used with PIECE_COUNT_SCORE.
Score win_score(const Position &pos);

// The pieces that would complete a line (or 2x2 square, when squares win) if
// placed now: one line of three sharing a property is enough. Table lookups
// only, see COMPLETES_TABLE.
Bitboard winning_pieces(Position &pos);

// Static score for the side to move.
//
// THREAT_EVAL: in a position where the piece in hand cannot win (negamax
// checks hand_wins first), -(win_score(pos) - 1) if every placement + give
// lets the opponent win at once (with one more piece on the board), else 0.
//
// Otherwise: a win (WIN, or win_score with PIECE_COUNT_SCORE) if the piece in
// hand can win on some cell, else 0.
Score eval(Position &pos);

} // namespace quarto
