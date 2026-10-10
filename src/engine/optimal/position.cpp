#include "position.hpp"
#include <bit>

namespace quarto {

void Position::load(Bitboard occ, std::array<Bitboard, 4> prop,
                    Piece hand) {
  occ_ = occ;
  prop_ = prop;
  hand_ = hand;
  top_ = 0;
  avail_ = 0xFFFF;
  if (hand_ >= 0) {
    avail_ &= ~(Bitboard{1} << hand_); // the piece in hand cannot be given again
  }

  while (occ) {
    int square = std::countr_zero(occ);

    Piece piece = 0;
    for (int i = 0; i < 4; ++i) {
      if ((prop_[i] >> square) & 1) {
        piece |= 1 << i;
      }
    }
    avail_ &= ~(Bitboard{1} << piece);
    occ &= occ - 1; // remove the lowest 1 bit
  }
}

void Position::place_piece(Cell c) {
  history_[top_++] = {c, hand_};
  if (c >= 0 && hand_ >= 0) {
    Bitboard mask = Bitboard{1} << c;
    occ_ |= mask;
    for (int i = 0; i < 4; ++i) {
      if ((hand_ >> i) & 1) {
        prop_[i] |= mask;
      }
    }
  }
}

void Position::give_piece(Piece p) {
  if (p >= 0) {
    avail_ &= ~(Bitboard{1} << p);
  }
  hand_ = p;
}

void Position::undo_place_piece() {
  Undo u = history_[--top_];
  if (u.cell >= 0 && u.hand >= 0) {
    Bitboard mask = ~(Bitboard{1} << u.cell);
    occ_ &= mask;
    for (int i = 0; i < 4; ++i) {
      if ((u.hand >> i) & 1) {
        prop_[i] &= mask;
      }
    }
  }
}

void Position::undo_give_piece() {
  if (hand_ >= 0) {
    avail_ |= Bitboard{1} << hand_;
  }

  // At the opening the root gives without placing first, so there is no
  // history entry to restore from - the hand simply goes back to empty.
  Piece prev = (top_ == 0) ? Piece(-1) : history_[top_ - 1].hand;
  hand_ = prev;
}

} // namespace quarto
