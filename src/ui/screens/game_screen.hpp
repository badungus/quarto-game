// game_screen.hpp - Declares GameScreen: composes all game views and turns
// input into Commands.
#pragma once
#include "assets.hpp"
#include "button.hpp"
#include "game_state.hpp"
#include "input.hpp"
#include "match_controller.hpp"
#include "nav.hpp"
#include "types.hpp"
#include <array>
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
  // Per player: engine output lines scrolled up from the end, and the line
  // count last drawn, so a scrolled-back view stays put as lines arrive.
  std::array<int, 2> output_scroll_{};
  std::array<int, 2> output_lines_{};
  void draw_engine_output(const std::array<std::vector<EngineInfo>, 17> &info,
                          float x, int player);

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
