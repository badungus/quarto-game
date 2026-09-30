// screen.hpp - Declares Screen: owns the screens, tracks which one is active
// and forwards update(input) and draw() to it.
#pragma once
#include "../../game/game_state.hpp"
#include "../../match/command.hpp"
#include "../../match/match_controller.hpp"
#include "../input.hpp"
#include "../theme/assets.hpp"
#include "game_screen.hpp"
#include <optional>

enum class ScreenId { Menu, Game, Result };

class Screen {
private:
  Game game;
  std::optional<Command> command;
  MatchConfig match_config{EngineSeat{"Thor", 8, 3000}, HumanSeat{}};
  MatchController match_controller{game, match_config, command};
  GameScreen game_screen_;
  ScreenId current_ = ScreenId::Game;

public:
  Screen(const Assets &assets);
  void update(const Input &in);
  void draw();
};
