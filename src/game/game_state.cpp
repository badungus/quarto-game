#include "game_state.hpp"

Game::Game() {
  board_.fill(-1);
  avail_.fill(true);
  hand_ = -1;
  top_ = 0;
  winning_line_.fill(-1);
  current_player_ = PlayerID::ONE;
  history_.fill({-1, -1});
}

bool Game::place_piece(Cell c) {
  if (board_[c] == -1 && hand_ >= 0) {
    history_[top_].cell = c;
    history_[top_].hand = -1; // a new move ends the redo history
    board_[c] = hand_;
    piece_placed_ = true;
    hand_ = -1;
    if (check_win()) {
      win_ = true;
    } else if (check_draw()) {
      draw_ = true;
    }
    return true;
  }
  return false;
}

bool Game::give_hand(Piece p) {
  if (p < 0 || !avail_[p])
    return false;
  history_[top_++].hand = p;
  history_[top_].cell = -1; // a new move ends the redo history
  hand_ = p;
  avail_[p] = false;
  piece_placed_ = false;
  current_player_ =
      current_player_ == PlayerID::ONE ? PlayerID::TWO : PlayerID::ONE;
  return true;
}

// history_[k] = {cell the previous piece was placed on, piece given at k}.
// top_ is one past the last give, so a placement in progress lives in
// history_[top_].cell.
void Game::unplace_piece() {
  if (top_ <= 0 || !piece_placed_ || hand_ >= 0 || history_[top_].cell < 0)
    return;
  Cell c = history_[top_].cell;
  board_[c] = -1;
  hand_ = history_[top_ - 1].hand;
  piece_placed_ = false;
  win_ = false;
  draw_ = false;
  winning_line_.fill(-1);
}

void Game::ungive_hand() {
  if (top_ <= 0 || piece_placed_ || hand_ < 0)
    return;

  --top_;
  current_player_ =
      current_player_ == PlayerID::ONE ? PlayerID::TWO : PlayerID::ONE;
  avail_[hand_] = true;
  hand_ = -1;
  piece_placed_ = true;
}

// Redo replays the stored move, then restores the next entry that the move
// cleared as if it were new.
void Game::redo_piece() {
  Piece next = history_[top_].hand;
  if (piece_placed_ || history_[top_].cell < 0 ||
      !place_piece(history_[top_].cell))
    return;
  history_[top_].hand = next;
}

void Game::redo_hand() {
  if (!piece_placed_ || history_[top_].hand < 0)
    return;
  Cell next = history_[top_ + 1].cell;
  give_hand(history_[top_].hand);
  history_[top_].cell = next;
}

bool Game::check_draw() {
  for (int i = 0; i < 16; ++i) {
    if (board_[i] < 0)
      return false;
  }

  return true;
}

bool Game::check_win() {
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
    if (board_[line[0]] < 0 || board_[line[1]] < 0 || board_[line[2]] < 0 ||
        board_[line[3]] < 0)
      continue;
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

void Game::restart() {

  board_.fill(-1);
  avail_.fill(true);
  hand_ = -1;
  top_ = 0;
  winning_line_.fill(-1);
  current_player_ = PlayerID::ONE;
  piece_placed_ = true;
  win_ = false;
  draw_ = false;
  history_.fill({-1, -1});
};
