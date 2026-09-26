// match_controller.hpp - Declares MatchController: owns GameState, History and
// two Players; applies Commands and drives turns.
#pragma once
#include "../game/game_state.hpp"
#include "../players/player.hpp"
#include "match_config.hpp"

class MatchController {
private:
  Game &game_;

  bool piece_placed_;

  HumanSeat human_1{};

  EngineSeat engine_1{"Thor", 3, 2000};

  MatchConfig match_config{
      human_1,
      engine_1,
  };
  Player player_1;

  Player player_2;

  bool update_player(Player &player);

public:
  MatchController(Game &game);
  void update();
};
