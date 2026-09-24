#pragma once
#include "types.hpp"

class Game {
  Board board;
  Piece hand;
  Avail avail;
  bool current_player;
  std::array<Undo, 17> history;
  int top;
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
};
