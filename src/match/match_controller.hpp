// match_controller.hpp - Declares MatchController: owns GameState, History and
// two Players; applies Commands and drives turns.
#pragma once
#include "../game/game_state.hpp"
#include "../players/player.hpp"
#include "command.hpp"
#include "match_config.hpp"
#include <memory>

class MatchController {
private:
  Game &game_;

  bool piece_placed_;

  MatchConfig match_config_;
  std::unique_ptr<Player> make_player(const Seat &seat, const Game &game);
  std::unique_ptr<Player> player_1_;
  std::unique_ptr<Player> player_2_;
  void start(const MatchConfig &config);
  bool update_player(Player &player);

public:
  MatchController(Game &game, MatchConfig &config,
                  std::optional<Command> &command)
      : game_(game) {
    start(config);
  };
  void update();
};
