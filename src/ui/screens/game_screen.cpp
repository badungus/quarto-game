// game_screen.cpp - Implements GameScreen layout, input routing and reaction to
// match events.
#include "game_screen.hpp"
#include "../../game/types.hpp"
#include "../../match/command.hpp"
#include "../layout.hpp"
#include <optional>
#include <raylib.h>

GameScreen::GameScreen(Game &game, const Assets &assets)
    : game_(game), assets_(assets) {

  for (int i = 0; i < 16; ++i) {
    int col = i % 4;
    int row = i / 4;

    pool_buttons_.emplace_back(
        Rectangle{POOL_X + col * POOL_TILE, POOL_Y + row * POOL_TILE, POOL_TILE,
                  POOL_TILE},
        assets.pool_button_background, assets.pool_button_hover,
        &assets.pool_button_images[i]);
    board_buttons_.emplace_back(Rectangle{BOARD_X + col * BOARD_TILE,
                                          BOARD_Y + row * BOARD_TILE,
                                          BOARD_TILE, BOARD_TILE},
                                assets.pool_button_background,
                                assets.pool_button_hover, std::monostate{});
  }
}

std::optional<Command> GameScreen::update(const Input &in) {
  // board buttons
  std::optional<Command> command = std::nullopt;
  for (Cell i = 0; i < 16; ++i) {
    int piece = game_.board()[i];
    bool occ = (piece >= 0);
    board_buttons_[i].set_enabled(!occ);
    if (occ) {
      board_buttons_[i].set_content(&assets_.pool_button_images[piece]);
    }

    else {
      board_buttons_[i].set_content(std::monostate{});
    }
    if (board_buttons_[i].update(in)) {
      command = cmd::SelectCell{i};
    }
  }
  // pool buttons
  for (Piece i = 0; i < 16; ++i) {
    bool avail = game_.avail()[i];
    pool_buttons_[i].set_enabled(avail);
    if (!avail) {

      pool_buttons_[i].set_content(std::monostate{});
    } else {
      pool_buttons_[i].set_content(&assets_.pool_button_images[i]);
    }
    if (pool_buttons_[i].update(in)) {
      command = cmd::SelectGive{i};
    }
  }
  return command;
}

void GameScreen::draw_piece_pool() {

  for (int i = 0; i < 16; ++i) {
    pool_buttons_[i].draw();
  }
}

void GameScreen::draw_board() {
  for (int i = 0; i < 16; ++i) {
    board_buttons_[i].draw();
  }
}
