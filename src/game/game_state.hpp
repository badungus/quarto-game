#pragma once

#include <array>
#include <cstdint>

using Piece = std::int8_t;
using Board = std::array<Piece, 16>;
using Cell = std::int8_t;
using Avail = std::array<bool, 16>;
struct Move {
  Cell cell;
  Piece give;
};
struct Undo {
  Cell cell;
  Piece hand;
};

class Game {
  Board board;
  Piece hand;
  Avail avail;
  std::array<Undo, 17> history;
  int top;

public:
  Game();
  void make_move(Cell c, Piece p);
  void undo_move();
};
