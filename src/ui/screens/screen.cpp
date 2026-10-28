// screen.cpp - Implements Screen switching.
#include "screen.hpp"
#include "button.hpp"
#include "colors.hpp"
#include "layout.hpp"
#include "nav.hpp"
#include "raylib.h"
#include <optional>

Screen::Screen(const Assets &assets)
    : main_menu_(game, match_controller, match_config, assets),
      game_screen_(game, match_controller, assets), assets_(assets),
      engine_match_(game, match_controller, match_config, assets),
      home_button_(Rectangle{HOME_BUTTON_X(), HOME_BUTTON_Y, HOME_BUTTON_WIDTH,
                             HOME_BUTTON_HEIGHT},
                   assets.panel, colors::SECONDARY_BUTTON, "QUARTO", 2.0f) {};

void Screen::update(const Input &in) {
  ScreenId screen = current_;

  switch (current_) {
  case ScreenId::Menu: {
    Action action = main_menu_.update(in);
    // Apply the menu's command now; next frame the game screen overwrites it.
    command = action.cmd;
    if (command)
      match_controller.update();
    screen = action.screen;
    break;
  };
  case ScreenId::Game: {
    Action action = game_screen_.update(in);
    command = action.cmd;
    match_controller.update();
    screen = action.screen;
    break;
  }
  case ScreenId::Result: {
    Action action = engine_match_.update(in);
    command = action.cmd;
    // Outside a match, leave whatever game the menu set up alone.
    if (command || engine_match_.running())
      match_controller.update();
    screen = action.screen;
    break;
  }
  }

  if (home_button_.update(in)) {
    screen = ScreenId::Menu;
  }
  if (current_ == ScreenId::Result && screen != ScreenId::Result)
    engine_match_.stop();
  else if (current_ != ScreenId::Result && screen == ScreenId::Result)
    engine_match_.load_config();
  current_ = screen;
}

void Screen::draw() {
  draw_navbar();
  switch (current_) {
  case ScreenId::Menu:
    main_menu_.draw_menu_buttons();
    break;
  case ScreenId::Game:
    game_screen_.draw_piece_pool();
    game_screen_.draw_board();
    game_screen_.draw_player();
    game_screen_.draw_buttons();
    break;
  case ScreenId::Result:
    engine_match_.draw();
    break;
  }
}

void Screen::draw_navbar() {
  float s = ui_scale();
  DrawRectangleRec({0, 0, canvas_width() * s, NAVBAR_HEIGHT * s},
                   colors::SURFACE_HIGH);
  DrawRectangleRec({0, NAVBAR_HEIGHT * s - colors::OUTLINE_WIDTH,
                    canvas_width() * s, colors::OUTLINE_WIDTH},
                   colors::OUTLINE);
  home_button_.draw();
};
