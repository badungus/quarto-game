#include "game_state.hpp"

Game::Game() {

  board.fill(-1);
  avail.fill(true);
}

void Game::make_move(Cell c, Piece p) {
  history[top++] = {c, hand};
  board[c] = hand;
  hand = p;
  avail[p] = false;
}

void Game::undo_move() {
  Undo u = history[--top];
  board[u.cell] = -1;
  avail[hand] = true;
  hand = u.hand;
}
