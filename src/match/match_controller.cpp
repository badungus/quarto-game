// match_controller.cpp - Implements turn flow, human half-move buffering,
// engine job dispatch and match events for views.
#include "match_controller.hpp"
#include "../game/game_state.hpp"
#include "../game/types.hpp"
#include <optional>

MatchController::MatchController(Game &game)
    : game_(game), player_1(), player_2() {

      };

void MatchController::update() {
  PlayerID current = game_.current_player();
  bool turn_complete;
  switch (current) {
  case PlayerID::ONE:
    turn_complete = update_player(player_1);
    break;

  case PlayerID::TWO:
    turn_complete = update_player(player_2);
    break;
  }
  if (turn_complete) {
    game_.set_current_player(current == PlayerID::ONE ? PlayerID::TWO
                                                      : PlayerID::ONE);
    piece_placed_ = false;
  }
}

bool MatchController::update_player(Player &player) {
  std::optional<Cell> c = player.poll_cell();

  if (c.has_value()) {
    game_.place_piece(*c);
    piece_placed_ = true;
  }

  if (piece_placed_) {
    std::optional<Piece> p = player.poll_piece();
    if (p.has_value()) {
      game_.give_hand(*p);
      return true;
    }
  }
  return false;
}
