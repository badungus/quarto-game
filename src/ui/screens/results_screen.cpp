// results_screen.cpp - Implements EngineMatch.
#include "results_screen.hpp"
#include "button.hpp"
#include "command.hpp"
#include "game_state.hpp"
#include "layout.hpp"
#include "match_config.hpp"
#include "nav.hpp"
#include <algorithm>
#include <cmath>
#include <format>
#include <optional>
#include <raylib.h>

namespace {
// Top of the first log line, below the score header.
constexpr float LOG_Y =
    MATCH_OUTPUT_Y + MATCH_OUTPUT_PADDING + 2 * MATCH_OUTPUT_LINE;
int VISIBLE_LINES() {
  return (int)((MATCH_OUTPUT_Y + MATCH_OUTPUT_HEIGHT() - MATCH_OUTPUT_PADDING -
                LOG_Y) /
               MATCH_OUTPUT_LINE);
}

// Value of a digits-only TextField, or fallback when it is empty.
int field_value(const TextField &field, int fallback) {
  return field.value().empty() ? fallback : std::stoi(field.value());
}
} // namespace

EngineMatch::EngineMatch(Game &game, MatchController &match_controller,
                         MatchConfig &match_config, const Assets &assets)
    : game_(game), match_controller_(match_controller),
      match_config_(match_config), assets_(assets),
      start_({MATCH_COLUMN_1_X, MATCH_START_Y, 2 * PLAYER_DD_WIDTH,
              MATCH_START_HEIGHT},
             assets.pool_button_background, assets.pool_button_hover,
             assets.pool_button_armed, "Start"),
      games_field_(
          {MATCH_COLUMN_1_X, MATCH_FIELD_Y, PLAYER_DD_WIDTH, SETTING_HEIGHT},
          assets.pool_button_background, assets.pool_button_hover,
          assets.pool_button_armed, "GAMES", "100"),
      seed_field_(
          {MATCH_COLUMN_2_X, MATCH_FIELD_Y, PLAYER_DD_WIDTH, SETTING_HEIGHT},
          assets.pool_button_background, assets.pool_button_hover,
          assets.pool_button_armed, "SEED", "1"),
      player_1_dd_(
          {MATCH_COLUMN_1_X, PLAYER_DD_Y, PLAYER_DD_WIDTH, PLAYER_DD_HEIGHT},
          assets.pool_button_background, assets.pool_button_hover,
          assets.pool_button_armed, ENGINE_NAMES[0],
          std::vector<ButtonContent>(ENGINE_NAMES.begin(), ENGINE_NAMES.end())),
      player_2_dd_(
          {MATCH_COLUMN_2_X, PLAYER_DD_Y, PLAYER_DD_WIDTH, PLAYER_DD_HEIGHT},
          assets.pool_button_background, assets.pool_button_hover,
          assets.pool_button_armed, ENGINE_NAMES[1],
          std::vector<ButtonContent>(ENGINE_NAMES.begin(),
                                     ENGINE_NAMES.end())),
      rules_({MATCH_COLUMN_1_X, MATCH_RULES_Y, 2 * PLAYER_DD_WIDTH,
              SETTING_HEIGHT},
             assets.pool_button_background, assets.pool_button_hover,
             assets.pool_button_armed, "") {

  const float column_x[2] = {MATCH_COLUMN_1_X, MATCH_COLUMN_2_X};
  const int n = ENGINE_SETTINGS.size();
  for (int p = 0; p < 2; ++p) {
    for (int i = 0; i < n; ++i) {
      const EngineSetting &s = ENGINE_SETTINGS[i];
      Rectangle r = {column_x[p], SETTING_BOTTOM_Y - (n - i) * SETTING_HEIGHT,
                     PLAYER_DD_WIDTH, SETTING_HEIGHT};
      if (s.kind == SettingKind::Number)
        settings_ui_[p].emplace_back(
            std::in_place_type<TextField>, r, assets.pool_button_background,
            assets.pool_button_hover, assets.pool_button_armed, s.label,
            s.default_value);
      else
        settings_ui_[p].emplace_back(
            std::in_place_type<Button>, r, assets.pool_button_background,
            assets.pool_button_hover, assets.pool_button_armed, "");
    }
  }
};

Action EngineMatch::update(const Input &in) {
  // The setup is locked while the match runs, so the log stays consistent.
  if (!running_) {
    games_field_.update(in);
    seed_field_.update(in);
    for (int p = 0; p < 2; ++p) {
      for (size_t i = 0; i < ENGINE_SETTINGS.size(); ++i) {
        std::string &value = settings_[p][i];
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
      squares_ = !squares_;

    if (std::optional<int> idx = player_1_dd_.update(in))
      engines_[0] = *idx;
    if (std::optional<int> idx = player_2_dd_.update(in))
      engines_[1] = *idx;
  }

  rules_.set_content(squares_ ? "2X2 SQUARES ON" : "2X2 SQUARES OFF");

  int max_scroll = std::max(0, (int)log_.size() - VISIBLE_LINES());
  scroll_ = std::clamp(scroll_ + 3 * (int)GetMouseWheelMove(), 0, max_scroll);

  if (start_.update(in)) {
    if (!running_)
      return {begin(), ScreenId::Result};
    stop();
  } else if (running_) {
    if (const std::optional<GameResult> &result = match_controller_.result()) {
      record(*result);
      if (games_played_ < max_games_)
        return {start_game(), ScreenId::Result};
      end();
    }
  }
  return {std::nullopt, ScreenId::Result};
}

void EngineMatch::load_config() {
  const Seat *seats[2] = {&match_config_.player_1, &match_config_.player_2};
  Dropdown *dropdowns[2] = {&player_1_dd_, &player_2_dd_};
  squares_ = match_config_.squares;
  for (int p = 0; p < 2; ++p) {
    settings_[p] = match_config_.settings[p];
    // Toggles show settings_ on their next update; fields keep their own copy.
    for (size_t i = 0; i < ENGINE_SETTINGS.size(); ++i)
      if (auto *field = std::get_if<TextField>(&settings_ui_[p][i]))
        field->set_value(settings_[p][i]);

    if (const auto *seat = std::get_if<EngineSeat>(seats[p]))
      for (int k = 0; k < ENGINE_COUNT; ++k)
        if (ENGINES[k].executable == seat->executable) {
          engines_[p] = k;
          dropdowns[p]->set_content(ENGINES[k].name);
        }
  }
}

void EngineMatch::log(std::string line) {
  log_.push_back(std::move(line));
  // Keep a scrolled-back view where it is.
  if (scroll_ > 0)
    ++scroll_;
}

std::optional<Command> EngineMatch::begin() {
  max_games_ = field_value(games_field_, 0);
  if (max_games_ <= 0)
    return std::nullopt;
  rng_.seed(field_value(seed_field_, 1));
  saved_config_ = match_config_;
  games_played_ = draws_ = first_wins_ = second_wins_ = total_length_ = 0;
  wins_ = {};
  log_.clear();
  scroll_ = 0;
  running_ = true;
  start_.set_content("Stop");
  return start_game();
}

Command EngineMatch::start_game() {
  const int first = first_engine();
  const int seat_engine[2] = {first, 1 - first};
  Seat *seats[2] = {&match_config_.player_1, &match_config_.player_2};
  for (int seat = 0; seat < 2; ++seat) {
    *seats[seat] = ENGINES[engines_[seat_engine[seat]]];
    match_config_.settings[seat] = settings_[seat_engine[seat]];
    seeds_[seat] = rng_();
    match_config_.seeds[seat] = seeds_[seat];
  }
  match_config_.squares = squares_;
  return cmd::NewGame{};
}

void EngineMatch::record(const GameResult &result) {
  const Board &board = game_.board();
  const int length =
      std::count_if(board.begin(), board.end(), [](Piece p) { return p >= 0; });
  total_length_ += length;

  const int first = first_engine();
  std::string outcome = "draw";
  if (result.status == GameStatus::DRAW) {
    ++draws_;
  } else {
    const bool first_won = result.status == GameStatus::PLAYER_ONE_WIN;
    const int winner = first_won ? first : 1 - first;
    ++wins_[winner];
    ++(first_won ? first_wins_ : second_wins_);
    outcome = name(winner) + " wins";
  }
  ++games_played_;
  log(std::format("game {:3}: {} first, seeds [{}, {}], {} after {} "
                  "placements ({})",
                  games_played_, name(first), seeds_[0], seeds_[1], outcome,
                  length, result.reason));
}

void EngineMatch::stop() {
  if (!running_)
    return;
  match_controller_.set_active(false);
  log(std::format("stopped after {} of {} games", games_played_, max_games_));
  end();
}

// Prints the summary of the games played and hands the config back to the
// main menu.
void EngineMatch::end() {
  running_ = false;
  match_config_ = saved_config_;
  start_.set_content("Start");

  const int n = games_played_;
  if (n == 0)
    return;
  const int a = wins_[0], b = wins_[1], d = draws_;
  const double p = (a + d / 2.0) / n;
  log("");
  log(std::format("{} {} - {} {} - draws {}  ({} scores {:.1f}%)", name(0), a,
                  name(1), b, d, name(0), 100 * p));
  log(std::format("first player wins {}, second player wins {}, average length "
                  "{:.1f} placements",
                  first_wins_, second_wins_, (double)total_length_ / n));
  if (0 < p && p < 1) {
    auto elo = [](double score) { return -400 * std::log10(1 / score - 1); };
    // Standard error of the score from the per-game results.
    const double var =
        (a * (1 - p) * (1 - p) + d * (0.5 - p) * (0.5 - p) + b * p * p) / n;
    const double se = std::sqrt(var / n);
    auto clamp = [](double x) { return std::clamp(x, 1e-9, 1 - 1e-9); };
    log(std::format("Elo {} vs {}: {:+.0f}  (95% CI {:+.0f} .. {:+.0f})",
                    name(0), name(1), elo(p), elo(clamp(p - 1.96 * se)),
                    elo(clamp(p + 1.96 * se))));
  }
}

void EngineMatch::draw() {
  start_.draw();
  games_field_.draw();
  seed_field_.draw();
  for (const auto &column : settings_ui_)
    for (const auto &widget : column)
      std::visit([](const auto &w) { w.draw(); }, widget);
  if (!player_1_dd_.open() && !player_2_dd_.open())
    rules_.draw();
  // Last, so an open list covers the widgets below it.
  player_1_dd_.draw();
  player_2_dd_.draw();
  draw_output();
}

void EngineMatch::draw_output() const {
  float s = ui_scale();
  const Texture2D &background = assets_.pool_button_background;
  NPatchInfo patch = {{0, 0, (float)background.width, (float)background.height},
                      BORDER,
                      BORDER,
                      BORDER,
                      BORDER,
                      NPATCH_NINE_PATCH};
  DrawTextureNPatch(background, patch,
                    {MATCH_OUTPUT_X * s, MATCH_OUTPUT_Y * s,
                     MATCH_OUTPUT_WIDTH() * s, MATCH_OUTPUT_HEIGHT() * s},
                    {0, 0}, 0.0f, WHITE);

  const Color color = GetColor(0x1E1E1EFF);
  const int x = (int)((MATCH_OUTPUT_X + MATCH_OUTPUT_PADDING) * s);
  const int font_size = (int)(MATCH_OUTPUT_FONT * s);

  std::string header =
      std::format("{} {} - {} {} - draws {}", name(0), wins_[0], name(1),
                  wins_[1], draws_);
  if (running_)
    header += std::format("    game {} of {}", games_played_ + 1, max_games_);
  DrawText(header.c_str(), x,
           (int)((MATCH_OUTPUT_Y + MATCH_OUTPUT_PADDING) * s),
           (int)(1.4f * font_size), color);

  const int last = (int)log_.size() - scroll_;
  const int first = std::max(0, last - VISIBLE_LINES());
  for (int i = first; i < last; ++i)
    DrawText(log_[i].c_str(), x,
             (int)((LOG_Y + (i - first) * MATCH_OUTPUT_LINE) * s), font_size,
             color);
}
