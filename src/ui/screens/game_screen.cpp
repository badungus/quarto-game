// game_screen.cpp - Implements GameScreen layout, input routing and reaction to
// match events.
#include "game_screen.hpp"
#include "../../game/types.hpp"
#include "../../match/command.hpp"
#include "../../match/match_config.hpp"
#include "../../match/match_controller.hpp"
#include "../layout.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <raylib.h>
#include <string>
#include <variant>

GameScreen::GameScreen(Game &game, MatchController &match_controller,
                       const Assets &assets)
    : game_(game), match_controller_(match_controller), assets_(assets),

      undo_(Rectangle{DESIGN_WIDTH / 2 - 2.5f * BUTTON_WIDTH,
                      DESIGN_HEIGHT - SCREEN_BORDER - BUTTON_HEIGHT,
                      BUTTON_WIDTH, BUTTON_HEIGHT},
            assets.pool_button_background, assets.pool_button_hover,
            assets.pool_button_armed, &assets.undo),
      restart_(Rectangle{DESIGN_WIDTH / 2 - 1.5f * BUTTON_WIDTH,
                         DESIGN_HEIGHT - SCREEN_BORDER - BUTTON_HEIGHT,
                         BUTTON_WIDTH, BUTTON_HEIGHT},
               assets.pool_button_background, assets.pool_button_hover,
               assets.pool_button_armed, &assets.restart),

      pause_(Rectangle{DESIGN_WIDTH / 2 - 0.5f * BUTTON_WIDTH,
                       DESIGN_HEIGHT - SCREEN_BORDER - BUTTON_HEIGHT,
                       BUTTON_WIDTH, BUTTON_HEIGHT},
             assets.pool_button_background, assets.pool_button_hover,
             assets.pool_button_armed, &assets.pause),

      resume_(Rectangle{DESIGN_WIDTH / 2 + 0.5f * BUTTON_WIDTH,
                        DESIGN_HEIGHT - SCREEN_BORDER - BUTTON_HEIGHT,
                        BUTTON_WIDTH, BUTTON_HEIGHT},
              assets.pool_button_background, assets.pool_button_hover,
              assets.pool_button_armed, &assets.resume),

      redo_(Rectangle{DESIGN_WIDTH / 2 + 1.5f * BUTTON_WIDTH,
                      DESIGN_HEIGHT - SCREEN_BORDER - BUTTON_HEIGHT,
                      BUTTON_WIDTH, BUTTON_HEIGHT},
            assets.pool_button_background, assets.pool_button_hover,
            assets.pool_button_armed, &assets.redo) {

  // Row-major, so pool_buttons_[i] sits at col i % POOL_COLS, row i / POOL_COLS
  // and shows piece i.
  for (int row = 0; row < POOL_ROWS; ++row) {
    for (int col = 0; col < POOL_COLS; ++col) {
      pool_buttons_.emplace_back(
          Rectangle{POOL_X + col * POOL_TILE, POOL_Y + row * POOL_TILE,
                    POOL_TILE, POOL_TILE},
          assets.pool_button_background, assets.pool_button_hover,
          assets.pool_button_armed,
          &assets.pool_button_images[row * POOL_COLS + col]);
    }
  }
  for (int i = 0; i < 16; ++i) {
    int col = i % 4;
    int row = i / 4;
    board_buttons_.emplace_back(
        Rectangle{BOARD_X + col * BOARD_TILE, BOARD_Y + row * BOARD_TILE,
                  BOARD_TILE, BOARD_TILE},
        assets.pool_button_background, assets.pool_button_hover,
        assets.pool_button_armed, std::monostate{});
  }
}

std::optional<Command> GameScreen::update(const Input &in) {
  // board buttons
  std::optional<Command> command = std::nullopt;
  for (Cell i = 0; i < 16; ++i) {
    int piece = game_.board()[i];
    bool occ = (piece >= 0);
    board_buttons_[i].set_enabled(!occ && !game_.piece_placed() &&
                                  match_controller_.active() &&
                                  match_controller_.player_is_human());
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
    pool_buttons_[i].set_enabled(avail && game_.piece_placed() &&
                                 match_controller_.active() &&
                                 match_controller_.player_is_human());
    if (!avail && i != game_.hand()) {
      pool_buttons_[i].set_content(std::monostate{});
    } else {
      pool_buttons_[i].set_content(&assets_.pool_button_images[i]);
    }
    if (pool_buttons_[i].update(in)) {
      command = cmd::SelectGive{i};
    }
  }

  if (restart_.update(in))
    command = cmd::NewGame{};
  if (undo_.update(in))
    command = cmd::Undo{};
  if (pause_.update(in))
    command = cmd::Pause{};
  if (resume_.update(in))
    command = cmd::Resume{};
  if (redo_.update(in))
    command = cmd::Redo{};

  return command;
}

void GameScreen::draw_piece_pool() {

  for (int i = 0; i < 16; ++i) {
    pool_buttons_[i].draw();
  }
  Piece h = game_.hand();
  if (h < 0)
    return;
  float s = ui_scale();
  int col = h % POOL_COLS;
  int row = h / POOL_COLS;
  const Texture2D &tex = assets_.winning_cell;
  Rectangle dest = {(POOL_X + col * POOL_TILE) * s,
                    (POOL_Y + row * POOL_TILE) * s, POOL_TILE * s,
                    POOL_TILE * s};
  NPatchInfo patch = {{0, 0, (float)tex.width, (float)tex.height},
                      BORDER,
                      BORDER,
                      BORDER,
                      BORDER,
                      NPATCH_NINE_PATCH};
  DrawTextureNPatch(tex, patch, dest, {0, 0}, 0.0f, WHITE);
}

void GameScreen::draw_board() {
  float s = ui_scale();
  Texture2D background = assets_.board_background;
  NPatchInfo patch = {{0, 0, (float)background.width, (float)background.height},
                      BORDER,
                      BORDER,
                      BORDER,
                      BORDER,
                      NPATCH_NINE_PATCH};
  DrawTextureNPatch(background, patch,
                    {(BOARD_X - BOARD_PADDING) * s,
                     (BOARD_Y - BOARD_PADDING) * s,
                     (BOARD_TILE * 4 + 2 * BOARD_PADDING) * 2,
                     (BOARD_TILE * 4 + 2 * BOARD_PADDING) * s},
                    {0, 0}, 0.0f, WHITE);
  for (int i = 0; i < 16; ++i) {

    board_buttons_[i].draw();
  }
  if (game_.win()) {
    const Texture2D &tex = assets_.winning_cell;
    std::array<int, 4> winning_line = game_.winning_line();
    for (int i = 0; i < 4; ++i) {
      int col = winning_line[i] % 4;
      int row = winning_line[i] / 4;
      Rectangle dest = {(BOARD_X + col * BOARD_TILE) * s,
                        (BOARD_Y + row * BOARD_TILE) * s, BOARD_TILE * s,
                        BOARD_TILE * s};
      NPatchInfo patch = {{0, 0, (float)tex.width, (float)tex.height},
                          BORDER,
                          BORDER,
                          BORDER,
                          BORDER,
                          NPATCH_NINE_PATCH};
      DrawTextureNPatch(tex, patch, dest, {0, 0}, 0.0f, WHITE);
    }
  }
}

void GameScreen::draw_hand_slot() {
  const Texture2D &background = assets_.pool_button_background;
  float s = ui_scale();
  Rectangle dest = {HAND_X * s - HAND_SIZE * s / 2.0f,
                    HAND_Y * s - HAND_SIZE * s / 2.0f, s * HAND_SIZE,
                    s * HAND_SIZE};

  NPatchInfo patch = {{0, 0, (float)background.width, (float)background.height},
                      BORDER,
                      BORDER,
                      BORDER,
                      BORDER,
                      NPATCH_NINE_PATCH};
  DrawTextureNPatch(background, patch, dest, {0, 0}, 0.0f, WHITE);
  if (game_.hand() < 0)
    return;
  Texture2D content = assets_.pool_button_images[game_.hand()];

  DrawTexturePro(content, {0, 0, (float)content.width, (float)content.height},
                 dest, {0, 0}, 0.0f, WHITE);
};

void GameScreen::draw_player() {
  std::string name1 = std::visit([](const auto &seat) { return seat.name; },
                                 match_controller_.match_config().player_1);
  std::string name2 = std::visit([](const auto &seat) { return seat.name; },
                                 match_controller_.match_config().player_2);

  float s = ui_scale();
  Texture2D background = assets_.pool_button_background;
  NPatchInfo patch = {{0, 0, (float)background.width, (float)background.height},
                      BORDER,
                      BORDER,
                      BORDER,
                      BORDER,
                      NPATCH_NINE_PATCH};

  Rectangle dest_left = {PLAYER_1_X * s, PLAYER_Y * s, PLAYER_WIDTH * s,
                         PLAYER_HEIGHT * s};

  Rectangle dest_right = {PLAYER_2_X * s, PLAYER_Y * s, PLAYER_WIDTH * s,
                          PLAYER_HEIGHT * s};
  DrawTextureNPatch(background, patch, dest_left, {0, 0}, 0.0f, WHITE);
  DrawTextureNPatch(background, patch, dest_right, {0, 0}, 0.0f, WHITE);
  if (std::holds_alternative<HumanSeat>(
          match_controller_.match_config().player_1)) {
  } else if (std::holds_alternative<EngineSeat>(
                 match_controller_.match_config().player_1)) {

    std::vector<EngineInfo> player_1_history =
        match_controller_.engine_info().player_1_history;
    if (!player_1_history.empty()) {
      int16_t eval = player_1_history.back().score;
      int depth = player_1_history.back().depth;
      std::string move = player_1_history.back().move;
      float evalbar_height = EVALBAR_HEIGHT * ((eval + WIN) / (2.0f * WIN));
      Rectangle dest_eval_1 = {
          EVALBAR_1_X * s, ((EVALBAR_Y + EVALBAR_HEIGHT - evalbar_height)) * s,
          EVALBAR_WIDTH * s, evalbar_height * s};
      DrawTextureNPatch(background, patch, dest_eval_1, {0, 0}, 0.0f, WHITE);
    }
  };

  if (std::holds_alternative<HumanSeat>(
          match_controller_.match_config().player_2)) {
  } else if (std::holds_alternative<EngineSeat>(
                 match_controller_.match_config().player_2)) {

    std::vector<EngineInfo> player_2_history =
        match_controller_.engine_info().player_2_history;
    if (!player_2_history.empty()) {

      int16_t eval = player_2_history.back().score;
      int depth = player_2_history.back().depth;
      std::string move = player_2_history.back().move;
      float evalbar_height = EVALBAR_HEIGHT * ((eval + WIN) / (2.0f * WIN));
      Rectangle dest_eval_2 = {
          EVALBAR_2_X * s, ((EVALBAR_Y + EVALBAR_HEIGHT - evalbar_height)) * s,
          EVALBAR_WIDTH * s, evalbar_height * s};
      DrawTextureNPatch(background, patch, dest_eval_2, {0, 0}, 0.0f, WHITE);
    }
  };
};

void GameScreen::draw_buttons() {
  undo_.draw();
  restart_.draw();
  pause_.draw();
  resume_.draw();
  redo_.draw();
};
