#pragma once

#include "types.hpp"

class Game {
private:
  Board board_;
  Piece hand_;
  Avail avail_;
  bool current_player_;
  std::array<Undo, 17> history_;
  int top_;
  std::array<int, 4> winning_line_;

public:
  Game();

  void place_piece(Cell c);
  void give_hand(Piece p);
  void unplace_piece();
  void ungive_hand();

  bool win();
  bool draw();

  const std::array<int, 4> &winning_line() const { return winning_line_; }

  const Avail &avail() const { return avail_; }

  const Piece &hand() const { return hand_; }

  const Board &board() const { return board_; }
};
