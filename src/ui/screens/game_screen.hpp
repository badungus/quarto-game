// game_screen.hpp - Declares GameScreen: composes all game views and turns
// input into Commands.
#pragma once
#include "../../game/game_state.hpp"
#include "../widgets/button.hpp"
#include <array>
#include <raylib.h>
#include <vector>

class GameScreen {
private:
  std::array<Texture2D, 16> button_images;
  std::vector<Button> pool_buttons;
  Game &game_;

public:
  GameScreen(Game &game);
  void draw_analysis_view();
  void draw_board_view();
  void draw_hand_slot_view();
  void draw_piece_pool();
};
