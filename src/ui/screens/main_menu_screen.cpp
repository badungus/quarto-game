// main_menu_screen.cpp - Implements MainMenuScreen.

#include "main_menu_screen.hpp"
#include "button.hpp"
#include "command.hpp"
#include "layout.hpp"
#include "match_config.hpp"
#include "nav.hpp"
#include <optional>
#include <raylib.h>

MainMenu::MainMenu(Game &game, MatchController &match_controller,
                   MatchConfig &match_config, const Assets &assets)
    : game_(game), match_config_(match_config),
      start_({START_X(), START_Y, START_WIDTH, START_HEIGHT},
             assets.panel, colors::PRIMARY_BUTTON, "Start"),
      engine_match_({START_X(), MENU_MATCH_Y(), START_WIDTH, MENU_MATCH_HEIGHT},
                    assets.panel, colors::PRIMARY_BUTTON, "Engine Match"),
      player_1_dd_(
          {PLAYER_1_DD_X(), MENU_DD_Y(), PLAYER_DD_WIDTH, MENU_DD_HEIGHT},
          assets.panel, "PLAYER ONE",
          std::vector<ButtonContent>(PLAYERS.begin(), PLAYERS.end())),
      player_2_dd_(
          {PLAYER_2_DD_X(), MENU_DD_Y(), PLAYER_DD_WIDTH, MENU_DD_HEIGHT},
          assets.panel, "PLAYER TWO",
          std::vector<ButtonContent>(PLAYERS.begin(), PLAYERS.end())),
      rules_({START_X(), MENU_RULES_Y(), START_WIDTH, SETTING_HEIGHT},
             assets.panel, colors::SECONDARY_BUTTON, "") {
  const float column_x[2] = {PLAYER_1_DD_X(), PLAYER_2_DD_X()};
  const int n = ENGINE_SETTINGS.size();
  for (int p = 0; p < 2; ++p) {
    for (int i = 0; i < n; ++i) {
      const EngineSetting &s = ENGINE_SETTINGS[i];
      Rectangle r = {column_x[p],
                     MENU_SETTING_BOTTOM_Y() - (n - i) * SETTING_HEIGHT,
                     PLAYER_DD_WIDTH, SETTING_HEIGHT};
      if (s.kind == SettingKind::Number)
        settings_ui_[p].emplace_back(
            std::in_place_type<TextField>, r, assets.panel, s.label,
            s.default_value);
      else
        settings_ui_[p].emplace_back(
            std::in_place_type<Button>, r, assets.panel,
            colors::SECONDARY_BUTTON, "");
    }
  }
};

Action MainMenu::update(const Input &in) {
  for (int p = 0; p < 2; ++p) {
    for (size_t i = 0; i < ENGINE_SETTINGS.size(); ++i) {
      std::string &value = match_config_.settings[p][i];
      if (auto *field = std::get_if<TextField>(&settings_ui_[p][i])) {
        field->update(in);
        value = field->value();
      } else {
        Button &toggle = std::get<Button>(settings_ui_[p][i]);
        if (toggle.update(in))
          value = value.empty() ? "on" : "";
        toggle.set_content(ENGINE_SETTINGS[i].label +
                           (value.empty() ? " OFF" : " ON"));
      }
    }
  }

  // Before the dropdowns: a click that picks an entry of an open list must
  // not also reach the toggle under it.
  if (!player_1_dd_.open() && !player_2_dd_.open() && rules_.update(in))
    match_config_.squares = !match_config_.squares;
  rules_.set_content(match_config_.squares ? "2X2 SQUARES ON"
                                           : "2X2 SQUARES OFF");

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
  if (engine_match_.update(in))
    return {std::nullopt, ScreenId::Result};
  return {std::nullopt, ScreenId::Menu};
}

void MainMenu::draw_menu_buttons() {
  start_.draw();
  engine_match_.draw();
  if (!player_1_dd_.open() && !player_2_dd_.open())
    rules_.draw();
  player_1_dd_.draw();
  player_2_dd_.draw();
  for (const auto &column : settings_ui_)
    for (const auto &widget : column)
      std::visit([](const auto &w) { w.draw(); }, widget);
}
