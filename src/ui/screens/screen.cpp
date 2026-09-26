// screen.cpp - Implements Screen switching.
#include "screen.hpp"
#include "../../match/command.hpp"
#include "../../match/match_config.hpp"
#include <optional>

Screen::Screen(const Assets &assets)
    : game_screen_(game, assets),
      match_controller(game, match_config, command) {
  MatchConfig match_config;

  match_config.player_1 = HumanSeat{};
  match_config.player_2 = EngineSeat{"Thor", 3, 2000};
}

void Screen::update(const Input &in) {
  std::optional<Command> command;
  switch (current_) {
  case ScreenId::Game:
    command = game_screen_.update(in);
    break;
  default:
    break;
  }
}

void Screen::draw() {
  switch (current_) {
  case ScreenId::Game:
    game_screen_.draw_piece_pool();
    game_screen_.draw_board();
    break;
  default:
    break;
  }
}
