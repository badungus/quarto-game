// match_controller.cpp - Implements turn flow, human half-move buffering,
// engine job dispatch and match events for views.
#include "match_controller.hpp"
#include "command.hpp"
#include "engine_player.hpp"
#include "game_state.hpp"
#include "human_player.hpp"
#include "types.hpp"
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
    start(match_config_);
    active_ = true;
  } else if (command_ && std::holds_alternative<cmd::Stop>(*command_)) {
    player_1_->stop_search();
    player_2_->stop_search();
  } else if (command_ && std::holds_alternative<cmd::Pause>(*command_)) {
    active_ = false;
    return;
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
    update_player(*player_1_, current);
    break;

  case PlayerID::TWO:
    update_player(*player_2_, current);
    break;
  }

  if (result_)
    return; // forfeited
  if (game_.win()) {
    result_ = {current == PlayerID::ONE ? GameStatus::PLAYER_ONE_WIN
                                        : GameStatus::PLAYER_TWO_WIN,
               "win"};
    active_ = false;
  } else if (game_.draw()) {
    result_ = {GameStatus::DRAW, "board full"};
    active_ = false;
  }
}

// A human's illegal click is just ignored, but an engine that answers with an
// illegal or incomplete move would stall the game, so it loses instead.
void MatchController::update_player(Player &player, PlayerID id) {
  const bool engine = !player_is_human();
  if (!game_.piece_placed()) {
    std::optional<Cell> c = player.poll_cell();

    if (c.has_value()) {
      if (!game_.place_piece(*c) && engine)
        return forfeit(id, "illegal placement");
    } else if (engine && player.move_missing()) {
      return forfeit(id, "no placement");
    }
  }

  // The winning placement ends the game, nothing is given after it.
  if (game_.win() || game_.draw())
    return;

  if (game_.piece_placed()) {
    std::optional<Piece> p = player.poll_piece();
    if (p.has_value()) {
      if (!game_.give_hand(*p) && engine)
        return forfeit(id, "illegal give");
    } else if (engine && player.move_missing()) {
      return forfeit(id, "no give");
    }
  }
}

void MatchController::forfeit(PlayerID id, const std::string &reason) {
  result_ = {id == PlayerID::ONE ? GameStatus::PLAYER_TWO_WIN
                                 : GameStatus::PLAYER_ONE_WIN,
             reason};
  active_ = false;
}

// Steps back half moves and pauses the game if its the engines turn
void MatchController::undo() {
  player_1_->cancel();
  player_2_->cancel();
  result_.reset();

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
  result_.reset();

  game_.piece_placed() ? game_.redo_hand() : game_.redo_piece();
  if (!player_is_human()) {
    active_ = false;
  } else {

    active_ = !game_.win() && !game_.draw();
  }
};

void MatchController::start(const MatchConfig &config) {
  result_.reset();
  // Set once per game: the rules never change while it runs.
  game_.set_squares(config.squares);
  player_1_ = make_player(config.player_1, game_, config.go_args(0),
                          config.seeds[0], config.squares);
  player_2_ = make_player(config.player_2, game_, config.go_args(1),
                          config.seeds[1], config.squares);
}

std::unique_ptr<Player>
MatchController::make_player(const Seat &seat, const Game &game,
                             const std::string &go_args,
                             std::optional<std::uint64_t> seed, bool squares) {
  return std::visit(overloaded{
                        [this](const HumanSeat &) -> std::unique_ptr<Player> {
                          return std::make_unique<HumanPlayer>(command_);
                        },
                        [&](const EngineSeat &s) -> std::unique_ptr<Player> {
                          std::vector<std::string> args;
                          if (squares && s.squares_flag)
                            args.push_back("--squares");
                          return std::make_unique<EnginePlayer>(
                              game, s.executable, s.executable, args, go_args,
                              seed);
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
