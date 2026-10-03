// main_menu_screen.cpp - Implements MainMenuScreen.

#include "main_menu_screen.hpp"
#include "button.hpp"
#include "command.hpp"
#include "layout.hpp"
#include "match_config.hpp"
#include "nav.hpp"
#include "screen.hpp"
#include <optional>
#include <raylib.h>

MainMenu::MainMenu(Game &game, MatchController &match_controller,
                   MatchConfig &match_config, const Assets &assets)
    : game_(game), match_config_(match_config),
      start_({START_X, START_Y, START_WIDTH, START_HEIGHT},
             assets.pool_button_background, assets.pool_button_hover,
             assets.pool_button_armed, "Start"),
      player_1_dd_(
          {PLAYER_1_DD_X, PLAYER_DD_Y, PLAYER_DD_WIDTH, PLAYER_DD_HEIGHT},
          assets.pool_button_background, assets.pool_button_hover,
          assets.pool_button_armed, "PLAYER ONE",
          std::vector<ButtonContent>(PLAYERS.begin(), PLAYERS.end())),
      player_2_dd_(
          {PLAYER_2_DD_X, PLAYER_DD_Y, PLAYER_DD_WIDTH, PLAYER_DD_HEIGHT},
          assets.pool_button_background, assets.pool_button_hover,
          assets.pool_button_armed, "PLAYER TWO",
          std::vector<ButtonContent>(PLAYERS.begin(), PLAYERS.end())) {};

Action MainMenu::update(const Input &in) {
  std::optional<int> player_1_idx = player_1_dd_.update(in);
  std::optional<int> player_2_idx = player_2_dd_.update(in);

  if (player_1_idx.has_value()) {
    if (player_1_idx == 0) {
      match_config_.player_1 = HumanSeat{"PLAYER"};
    } else {
      match_config_.player_1 = ENGINES[*player_1_idx - 1];
    }
  }

  if (player_2_idx.has_value()) {
    if (player_2_idx == 0) {
      match_config_.player_2 = HumanSeat{"PLAYER"};
    } else {
      match_config_.player_2 = ENGINES[*player_2_idx - 1];
    }
  }

  if (start_.update(in))
    return {cmd::NewGame{}, ScreenId::Game};
  return {std::nullopt, ScreenId::Menu};
}

void MainMenu::draw_menu_buttons() {
  start_.draw();
  player_1_dd_.draw();
  player_2_dd_.draw();
}
