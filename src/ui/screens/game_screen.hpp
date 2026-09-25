// game_screen.hpp - Declares GameScreen: composes all game views and turns
// input into Commands.
#pragma once
#include "../../game/game_state.hpp"
#include "../input.hpp"
#include "../theme/assets.hpp"
#include "../widgets/button.hpp"
#include <vector>

class GameScreen {
private:
  std::vector<Button> pool_buttons_;
  Game &game_;

public:
  GameScreen(Game &game, const Assets &assets);
  void update(const Input &in);
  void draw_analysis_view();
  void draw_board_view();
  void draw_hand_slot_view();
  void draw_piece_pool();
};
