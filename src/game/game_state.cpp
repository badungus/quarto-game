#include "game_state.hpp"

Game::Game() {
  board_.fill(-1);
  avail_.fill(true);
}

void Game::place_piece(Cell c) {
  history_[top_++] = {c, hand_};
  board_[c] = hand_;
}

void Game::give_hand(Piece p) {
  hand_ = p;
  avail_[p] = false;
}

void Game::unplace_piece() {
  Undo u = history_[top_ - 1];
  board_[u.cell] = -1;
}

void Game::ungive_hand() {
  Undo u = history_[--top_];
  avail_[hand_] = true;
  hand_ = u.hand;
}

bool Game::draw() {
  for (int i = 0; i < 16; ++i) {
    if (board_[i] < 0)
      return false;
  }

  return true;
}

bool Game::win() {
  std::array<std::array<int, 4>, 10> lines = {{{0, 1, 2, 3},
                                               {4, 5, 6, 7},
                                               {8, 9, 10, 11},
                                               {12, 13, 14, 15},
                                               {0, 4, 8, 12},
                                               {1, 5, 9, 13},
                                               {2, 6, 10, 14},
                                               {3, 7, 11, 15},
                                               {0, 5, 10, 15},
                                               {3, 6, 9, 12}}};

  for (const auto &line : lines) {
    for (int i = 0; i < 4; ++i) {
      bool all_ones = ((board_[line[0]] & board_[line[1]] & board_[line[2]] &
                        board_[line[3]]) >>
                       i) &
                      1;

      bool all_zeros = ((~board_[line[0]] & ~board_[line[1]] &
                         ~board_[line[2]] & ~board_[line[3]]) >>
                        i) &
                       1;

      if (all_ones || all_zeros) {
        winning_line_ = line;
        return true;
      }
    }
  }

  return false;
}
