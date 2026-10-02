// match_controller.cpp - Implements turn flow, human half-move buffering,
// engine job dispatch and match events for views.
#include "match_controller.hpp"
#include "../game/game_state.hpp"
#include "../game/types.hpp"
#include "../players/engine_player.hpp"
#include "../players/human_player.hpp"
#include <optional>
#include <variant>

namespace {
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
} // namespace

void MatchController::update() {
  // Refresh both every frame, also while paused or after undo/redo, so the
  // views never draw an outdated copy.
  engine_info_history_.player_1_history = player_1_->engine_info();
  engine_info_history_.player_2_history = player_2_->engine_info();

  if (command_ && std::holds_alternative<cmd::Undo>(*command_)) {
    undo();
    return;
  } else if (command_ && std::holds_alternative<cmd::NewGame>(*command_)) {
    player_1_->cancel();
    player_2_->cancel();
    game_.restart();
    active_ = true;
  } else if (command_ && std::holds_alternative<cmd::Pause>(*command_)) {
    active_ = false;
  } else if (command_ && std::holds_alternative<cmd::Resume>(*command_)) {
    active_ = true;
  } else if (command_ && std::holds_alternative<cmd::Redo>(*command_)) {
    redo();
    return;
  } else if (!active_)
    return;
  PlayerID current = game_.current_player();
  switch (current) {
  case PlayerID::ONE:
    update_player(*player_1_);
    break;

  case PlayerID::TWO:
    update_player(*player_2_);
    break;
  }

  if (game_.win()) {
    game_status_ = current == PlayerID::ONE ? GameStatus::PLAYER_ONE_WIN
                                            : GameStatus::PLAYER_TWO_WIN;
    active_ = false;
  } else if (game_.draw()) {
    game_status_ = GameStatus::DRAW;
    active_ = false;
  }
}

void MatchController::update_player(Player &player) {
  if (!game_.piece_placed()) {
    std::optional<Cell> c = player.poll_cell();

    if (c.has_value()) {
      game_.place_piece(*c);
    }
  }

  if (game_.piece_placed()) {
    std::optional<Piece> p = player.poll_piece();
    if (p.has_value()) {
      game_.give_hand(*p);
    }
  }
}

// Steps back half moves and pauses the game if its the engines turn
void MatchController::undo() {
  player_1_->cancel();
  player_2_->cancel();

  game_.piece_placed() ? game_.unplace_piece() : game_.ungive_hand();
  if (!player_is_human()) {
    active_ = false;
  } else {

    active_ = !game_.win() && !game_.draw();
  }
}

void MatchController::redo() {
  player_1_->cancel();
  player_2_->cancel();

  game_.piece_placed() ? game_.redo_hand() : game_.redo_piece();
  if (!player_is_human()) {
    active_ = false;
  } else {

    active_ = !game_.win() && !game_.draw();
  }
};

void MatchController::start(const MatchConfig &config) {
  player_1_ = make_player(config.player_1, game_);
  player_2_ = make_player(config.player_2, game_);
}

std::unique_ptr<Player> MatchController::make_player(const Seat &seat,
                                                     const Game &game) {
  return std::visit(overloaded{
                        [this](const HumanSeat &) -> std::unique_ptr<Player> {
                          return std::make_unique<HumanPlayer>(command_);
                        },
                        [&](const EngineSeat &s) -> std::unique_ptr<Player> {
                          return std::make_unique<EnginePlayer>(
                              game, s.name, s.executable, s.search_depth,
                              s.time_limit);
                        },
                    },
                    seat);
}

const bool MatchController::player_is_human() {
  const Seat &seat = game_.current_player() == PlayerID::ONE
                         ? match_config_.player_1
                         : match_config_.player_2;

  return std::holds_alternative<HumanSeat>(seat);
};
