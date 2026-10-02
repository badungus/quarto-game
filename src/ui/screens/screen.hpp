// screen.hpp - Declares Screen: owns the screens, tracks which one is active
// and forwards update(input) and draw() to it.
#pragma once
#include "game_state.hpp"
#include "command.hpp"
#include "match_controller.hpp"
#include "input.hpp"
#include "main_menu_screen.hpp"
#include "assets.hpp"
#include "game_screen.hpp"
#include <optional>

enum class ScreenId { Menu, Game, Result };

class Screen {
private:
  Game game;
  std::optional<Command> command;
  MatchConfig match_config{HumanSeat{"Player"},
                           EngineSeat{"Thor", "thor", 16, 1000}};
  MatchController match_controller{game, match_config, command};
  MainMenu main_menu_;
  GameScreen game_screen_;
  ScreenId current_ = ScreenId::Game;
  const Assets &assets_;

  void draw_navbar();

public:
  Screen(const Assets &assets);
  void update(const Input &in);
  void draw();
};
