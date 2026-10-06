// game_screen.cpp - Implements GameScreen layout, input routing and reaction to
// match events.
#include "game_screen.hpp"
#include "command.hpp"
#include "game_state.hpp"
#include "layout.hpp"
#include "match_config.hpp"
#include "match_controller.hpp"
#include "types.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <raylib.h>
#include <string>
#include <variant>

namespace {
// An engine only searches on its own turns, so walk back from the current top
// to its latest search at or before this position.
const EngineInfo *
latest_info(const std::array<std::vector<EngineInfo>, 17> &history, int top) {
  for (int k = top; k >= 0; --k) {
    if (!history[k].empty())
      return &history[k].back();
  }
  return nullptr;
}

// The info line as the engine sent it, so keywords the GUI does not parse are
// shown too.
std::string info_text(const EngineInfo &info) {
  const std::string prefix = "info ";
  return info.raw.starts_with(prefix) ? info.raw.substr(prefix.size())
                                      : info.raw;
}
} // namespace

GameScreen::GameScreen(Game &game, MatchController &match_controller,
                       const Assets &assets)
    : game_(game), match_controller_(match_controller), assets_(assets),

      undo_(Rectangle{DESIGN_WIDTH / 2 - (0.5f * BUTTON_COUNT * BUTTON_WIDTH),
                      DESIGN_HEIGHT - BORDER - BUTTON_HEIGHT, BUTTON_WIDTH,
                      BUTTON_HEIGHT},
            assets.pool_button_background, assets.pool_button_hover,
            assets.pool_button_armed, &assets.undo),
      restart_(Rectangle{DESIGN_WIDTH / 2 -
                             (0.5f * (BUTTON_COUNT - 2)) * BUTTON_WIDTH,
                         DESIGN_HEIGHT - BORDER - BUTTON_HEIGHT, BUTTON_WIDTH,
                         BUTTON_HEIGHT},
               assets.pool_button_background, assets.pool_button_hover,
               assets.pool_button_armed, &assets.restart),

      stop_search_(Rectangle{DESIGN_WIDTH / 2 -
                                 (0.5f * (BUTTON_COUNT - 4) * BUTTON_WIDTH),
                             DESIGN_HEIGHT - BORDER - BUTTON_HEIGHT,
                             BUTTON_WIDTH, BUTTON_HEIGHT},
                   assets.pool_button_background, assets.pool_button_hover,
                   assets.pool_button_armed, &assets.stop_search),
      pause_(Rectangle{DESIGN_WIDTH / 2 -
                           (0.5f * (BUTTON_COUNT - 6) * BUTTON_WIDTH),
                       DESIGN_HEIGHT - BORDER - BUTTON_HEIGHT, BUTTON_WIDTH,
                       BUTTON_HEIGHT},
             assets.pool_button_background, assets.pool_button_hover,
             assets.pool_button_armed, &assets.pause),

      resume_(Rectangle{DESIGN_WIDTH / 2 -
                            (0.5f * (BUTTON_COUNT - 8) * BUTTON_WIDTH),
                        DESIGN_HEIGHT - BORDER - BUTTON_HEIGHT, BUTTON_WIDTH,
                        BUTTON_HEIGHT},
              assets.pool_button_background, assets.pool_button_hover,
              assets.pool_button_armed, &assets.resume),

      redo_(Rectangle{DESIGN_WIDTH / 2 -
                          (0.5f * (BUTTON_COUNT - 10) * BUTTON_WIDTH),
                      DESIGN_HEIGHT - BORDER - BUTTON_HEIGHT, BUTTON_WIDTH,
                      BUTTON_HEIGHT},
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

Action GameScreen::update(const Input &in) {
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
  if (stop_search_.update(in)) {
    command = cmd::Stop{};
  }
  if (resume_.update(in))
    command = cmd::Resume{};
  if (redo_.update(in))
    command = cmd::Redo{};

  return {command, ScreenId::Game};
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
  const Texture2D &content = assets_.pool_button_images[game_.hand()];

  DrawTexturePro(content, {0, 0, (float)content.width, (float)content.height},
                 dest, {0, 0}, 0.0f, WHITE);
};

void GameScreen::draw_player() {
  std::string name_1 = std::visit([](const auto &seat) { return seat.name; },
                                  match_controller_.match_config().player_1);
  std::string name_2 = std::visit([](const auto &seat) { return seat.name; },
                                  match_controller_.match_config().player_2);

  float s = ui_scale();
  const Texture2D &background = assets_.pool_button_background;
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

  const Texture2D &name_background = assets_.pool_button_hover;

  NPatchInfo name_patch = {
      {0, 0, (float)name_background.width, (float)name_background.height},
      BORDER,
      BORDER,
      BORDER,
      BORDER,
      NPATCH_NINE_PATCH};
  Rectangle name_1_dest = {PLAYER_1_NAME_X * s, PLAYER_NAME_Y * s,
                           PLAYER_NAME_WIDTH * s, PLAYER_NAME_HEIGHT * s};
  Rectangle name_2_dest = {PLAYER_2_NAME_X * s, PLAYER_NAME_Y * s,
                           PLAYER_NAME_WIDTH * s, PLAYER_NAME_HEIGHT * s};
  DrawTextureNPatch(name_background, name_patch, name_1_dest, {0, 0}, 0.0f,
                    WHITE);
  DrawTextureNPatch(name_background, name_patch, name_2_dest, {0, 0}, 0.0f,
                    WHITE);
  Rectangle current_player_marker_dest =
      game_.current_player() == PlayerID::ONE ? name_1_dest : name_2_dest;
  const Texture2D &tex = assets_.winning_cell;
  NPatchInfo current_player_marker_patch = {
      {0, 0, (float)tex.width, (float)tex.height},
      BORDER,
      BORDER,
      BORDER,
      BORDER,
      NPATCH_NINE_PATCH};
  DrawTextureNPatch(tex, current_player_marker_patch,
                    current_player_marker_dest, {0, 0}, 0.0f, WHITE);
  int name_1_font_size = (int)(name_1_dest.height * 0.3f);
  int name_1_text_width = MeasureText(name_1.c_str(), name_1_font_size);
  DrawText(name_1.c_str(),
           (int)(name_1_dest.x + (name_1_dest.width - name_1_text_width) / 2),
           (int)(name_1_dest.y + (name_1_dest.height - name_1_font_size) / 2),
           name_1_font_size, GetColor(0x1E1E1EFF));
  int name_2_font_size = (int)(name_2_dest.height * 0.3f);
  int name_2_text_width = MeasureText(name_2.c_str(), name_2_font_size);
  DrawText(name_2.c_str(),
           (int)(name_2_dest.x + (name_2_dest.width - name_2_text_width) / 2),
           (int)(name_2_dest.y + (name_2_dest.height - name_2_font_size) / 2),
           name_2_font_size, GetColor(0x1E1E1EFF));

  Texture2D evalbar_background = assets_.evalbar_background;
  Texture2D evalbar = assets_.evalbar;
  NPatchInfo evalbar_patch = {
      {0, 0, (float)evalbar.width, (float)evalbar.height},
      BORDER,
      BORDER,
      BORDER,
      BORDER,
      NPATCH_NINE_PATCH};

  NPatchInfo evalbar_background_patch = {
      {0, 0, (float)evalbar_background.width, (float)evalbar_background.height},
      BORDER,
      BORDER,
      BORDER,
      BORDER,
      NPATCH_NINE_PATCH};
  if (std::holds_alternative<HumanSeat>(
          match_controller_.match_config().player_1)) {
  } else if (std::holds_alternative<EngineSeat>(
                 match_controller_.match_config().player_1)) {
    std::array<std::vector<EngineInfo>, 17> info_player_1 =
        match_controller_.engine_info().player_1_history;
    const EngineInfo *latest = latest_info(info_player_1, game_.top());
    int16_t eval = latest ? latest->score.value_or(0) : 0;
    Rectangle eval_1_background_dest = {EVALBAR_1_X * s, EVALBAR_Y * s,
                                        EVALBAR_WIDTH * s, EVALBAR_HEIGHT * s};
    float evalbar_height = EVALBAR_HEIGHT * ((eval + WIN) / (2.0f * WIN));
    Rectangle eval_1_dest = {
        EVALBAR_1_X * s, ((EVALBAR_Y + EVALBAR_HEIGHT - evalbar_height)) * s,
        EVALBAR_WIDTH * s, evalbar_height * s};
    DrawTextureNPatch(evalbar_background, evalbar_background_patch,
                      eval_1_background_dest, {0, 0}, 0.0f, WHITE);
    DrawTextureNPatch(evalbar, evalbar_patch, eval_1_dest, {0, 0}, 0.0f, WHITE);

    float engine_output_1_y = ENGINE_OUTPUT_Y;
    for (int i = 0; i < 17; ++i) {
      if (info_player_1[i].empty())
        continue;
      std::string new_move =
          "------------Move: " + std::to_string(i + 1) + "-------------";
      DrawText(new_move.c_str(), ENGINE_OUTPUT_PLAYER_1_X * s,
               engine_output_1_y * s, ENGINE_OUTPUT_SIZE * s,
               GetColor(0x1E1E1EFF));
      engine_output_1_y += 0.7 * UI_SPACING;
      for (auto &output : info_player_1[i]) {

        DrawText(info_text(output).c_str(), ENGINE_OUTPUT_PLAYER_1_X * s,
                 engine_output_1_y * s, ENGINE_OUTPUT_SIZE * s,
                 GetColor(0x1E1E1EFF));

        engine_output_1_y += 0.7f * UI_SPACING;
      }
    }
  };

  if (std::holds_alternative<HumanSeat>(
          match_controller_.match_config().player_2)) {
  } else if (std::holds_alternative<EngineSeat>(
                 match_controller_.match_config().player_2)) {
    std::array<std::vector<EngineInfo>, 17> info_player_2 =
        match_controller_.engine_info().player_2_history;
    const EngineInfo *latest = latest_info(info_player_2, game_.top());

    int16_t eval = latest ? latest->score.value_or(0) : 0;
    Rectangle eval_2_background_dest = {EVALBAR_2_X * s, EVALBAR_Y * s,
                                        EVALBAR_WIDTH * s, EVALBAR_HEIGHT * s};
    float evalbar_height = EVALBAR_HEIGHT * ((eval + WIN) / (2.0f * WIN));
    Rectangle eval_2_dest = {
        EVALBAR_2_X * s, ((EVALBAR_Y + EVALBAR_HEIGHT - evalbar_height)) * s,
        EVALBAR_WIDTH * s, evalbar_height * s};
    DrawTextureNPatch(evalbar_background, evalbar_background_patch,
                      eval_2_background_dest, {0, 0}, 0.0f, WHITE);
    DrawTextureNPatch(evalbar, evalbar_patch, eval_2_dest, {0, 0}, 0.0f, WHITE);

    float engine_output_2_y = ENGINE_OUTPUT_Y;
    for (int i = 0; i < 17; ++i) {
      if (info_player_2[i].empty())
        continue;
      std::string new_move =
          "------------Move: " + std::to_string(i + 1) + "-------------";
      DrawText(new_move.c_str(), ENGINE_OUTPUT_PLAYER_2_X * s,
               engine_output_2_y * s, ENGINE_OUTPUT_SIZE * s,
               GetColor(0x1E1E1EFF));
      engine_output_2_y += 0.7 * UI_SPACING;
      for (auto &output : info_player_2[i]) {

        DrawText(info_text(output).c_str(), ENGINE_OUTPUT_PLAYER_2_X * s,
                 engine_output_2_y * s, ENGINE_OUTPUT_SIZE * s,
                 GetColor(0x1E1E1EFF));

        engine_output_2_y += 0.7f * UI_SPACING;
      }
    }
  };
};

void GameScreen::draw_buttons() {
  undo_.draw();
  restart_.draw();
  stop_search_.draw();
  pause_.draw();
  resume_.draw();
  redo_.draw();
};
