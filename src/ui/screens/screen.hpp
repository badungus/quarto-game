// screen.hpp - Declares Screen: owns the screens, tracks which one is active
// and forwards update(input) and draw() to it.
#pragma once
#include "assets.hpp"
#include "command.hpp"
#include "game_screen.hpp"
#include "game_state.hpp"
#include "input.hpp"
#include "main_menu_screen.hpp"
#include "match_controller.hpp"
#include "nav.hpp"
#include "results_screen.hpp"
#include <optional>

class Screen {
private:
  Game game;
  std::optional<Command> command;
  MatchConfig match_config{HumanSeat{"Player"}, EngineSeat{"THOR", "thor"}};
  MatchController match_controller{game, match_config, command};
  MainMenu main_menu_;
  GameScreen game_screen_;
  EngineMatch engine_match_;
  ScreenId current_ = ScreenId::Menu;
  const Assets &assets_;
  Button home_button_;

  void draw_navbar();

public:
  Screen(const Assets &assets);
  void update(const Input &in);
  void draw();
  void set_screen(ScreenId s) { current_ = s; }
};
