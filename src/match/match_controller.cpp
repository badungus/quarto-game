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
  if (!active_)
    return;
  PlayerID current = game_.current_player();
  bool turn_complete = false;
  switch (current) {
  case PlayerID::ONE:
    turn_complete = update_player(*player_1_);
    break;

  case PlayerID::TWO:
    turn_complete = update_player(*player_2_);
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
  if (turn_complete) {
    game_.set_current_player(current == PlayerID::ONE ? PlayerID::TWO
                                                      : PlayerID::ONE);
    piece_placed_ = false;
  }
}

bool MatchController::update_player(Player &player) {
  if (!piece_placed_) {
    std::optional<Cell> c = player.poll_cell();

    if (c.has_value()) {
      if (game_.place_piece(*c))
        piece_placed_ = true;
    }
  }

  if (piece_placed_) {
    std::optional<Piece> p = player.poll_piece();
    if (p.has_value()) {
      if (game_.give_hand(*p))
        return true;
    }
  }
  return false;
}

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
                              game, s.engine_name, s.search_depth,
                              s.time_limit);
                        },
                    },
                    seat);
}
