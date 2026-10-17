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
  bool win_ = false;
  bool draw_ = false;
  bool piece_placed_ = true;
  bool squares_ = false; // 2x2 squares win too

  bool check_win();
  bool check_draw();

public:
  Game();

  bool place_piece(Cell c);
  bool give_hand(Piece p);
  void unplace_piece();
  void ungive_hand();
  void redo_piece();
  void redo_hand();
  void set_current_player(PlayerID current) { current_player_ = current; }
  void set_squares(bool squares) { squares_ = squares; }
  void restart();

  const bool win() { return win_; }
  const bool draw() { return draw_; }
  const std::array<int, 4> &winning_line() const { return winning_line_; }
  const Avail &avail() const { return avail_; }
  const Piece &hand() const { return hand_; }
  const Board &board() const { return board_; }
  const PlayerID &current_player() const { return current_player_; }
  const bool piece_placed() const { return piece_placed_; }
  const bool squares() const { return squares_; }
  const int top() const { return top_; }
};
