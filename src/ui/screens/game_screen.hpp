// game_screen.hpp - Declares GameScreen: composes all game views and turns
// input into Commands.
#pragma once
#include "assets.hpp"
#include "button.hpp"
#include "game_state.hpp"
#include "input.hpp"
#include "match_controller.hpp"
#include "nav.hpp"
#include <vector>

class GameScreen {
private:
  Game &game_;
  MatchController &match_controller_;
  const Assets &assets_;
  std::vector<Button> pool_buttons_;
  std::vector<Button> board_buttons_;
  Button restart_;
  Button undo_;
  Button redo_;
  Button pause_;
  Button resume_;
  Button stop_search_;

public:
  GameScreen(Game &game, MatchController &match_controller,
             const Assets &assets);
  Action update(const Input &in);
  void draw_analysis();
  void draw_board();
  void draw_hand_slot();
  void draw_piece_pool();
  void draw_player();
  void draw_buttons();
};
