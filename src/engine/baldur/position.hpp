#pragma once

#include <array>

#include "types.hpp"

namespace quarto {

class Position {
private:
  Bitboard occ_;                 // occupied cells
  std::array<Bitboard, 4> prop_; // cells containing a property
  Bitboard avail_;               // available pieces, neither placed nor in hand
  Piece hand_;                   // piece the side to move has to play
  std::array<Undo, 17> history_; // move history
  int top_;

public:
  void place_piece(Cell c);
  void give_piece(Piece p);
  void undo_place_piece();
  void undo_give_piece();

  void load(Bitboard occ, std::array<Bitboard, 4> prop, Piece hand);

  Bitboard empty_cells() const { return Bitboard(~occ_ & 0xFFFF); }
  Bitboard available() const { return avail_; }
  Bitboard occupied() const { return occ_; }
  Piece in_hand() const { return hand_; }

  const std::array<Bitboard, 4> &props() const { return prop_; }
};

} // namespace quarto
