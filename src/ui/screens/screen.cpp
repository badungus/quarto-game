// screen.cpp - Implements Screen switching.
#include "screen.hpp"
#include "layout.hpp"
#include "raylib.h"
#include <optional>

Screen::Screen(const Assets &assets)
    : main_menu_(game, match_controller, assets),
      game_screen_(game, match_controller, assets), assets_(assets) {};

void Screen::update(const Input &in) {
  switch (current_) {
  case ScreenId::Menu: {
    break;
  };
  case ScreenId::Game: {
    command = game_screen_.update(in);

    match_controller.update();
    break;
  }
  case ScreenId::Result: {

    break;
  }
  }
}

void Screen::draw() {
  draw_navbar();
  switch (current_) {
  case ScreenId::Game:
    game_screen_.draw_piece_pool();
    game_screen_.draw_board();
    game_screen_.draw_player();
    game_screen_.draw_buttons();
    break;
  default:
    break;
  }
}

void Screen::draw_navbar() {
  float s = ui_scale();
  Texture2D navbar = assets_.pool_button_background;

  NPatchInfo patch = {{0, 0, (float)navbar.width, (float)navbar.height},
                      BORDER,
                      BORDER,
                      BORDER,
                      BORDER,
                      NPATCH_NINE_PATCH};
  DrawTextureNPatch(navbar, patch, {0, 0, DESIGN_WIDTH * s, NAVBAR_HEIGHT * s},
                    {0, 0}, 0.0f, WHITE);
};
