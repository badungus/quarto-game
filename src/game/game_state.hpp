#pragma once

#include "types.hpp"

enum class PlayerID { ONE, TWO };

class Game {
private:
  Board board_;
  Piece hand_;
  Avail avail_;
  PlayerID current_player_;
  std::array<Undo, 17> history_;
  int top_;
  std::array<int, 4> winning_line_;

public:
  Game();

  void place_piece(Cell c);
  void give_hand(Piece p);
  void unplace_piece();
  void ungive_hand();
  void set_current_player(PlayerID current) { current_player_ = current; }

  bool win();
  bool draw();

  const std::array<int, 4> &winning_line() const { return winning_line_; }

  const Avail &avail() const { return avail_; }

  const Piece &hand() const { return hand_; }

  const Board &board() const { return board_; }

  const PlayerID &current_player() const { return current_player_; }
};
