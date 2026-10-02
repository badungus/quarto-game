// match_controller.hpp - Declares MatchController: owns GameState, History and
// two Players; applies Commands and drives turns.
#pragma once
#include "game_state.hpp"
#include "player.hpp"
#include "command.hpp"
#include "match_config.hpp"
#include <memory>

enum class GameStatus { PLAYER_ONE_WIN, DRAW, PLAYER_TWO_WIN };

class MatchController {
private:
  Game &game_;
  GameStatus game_status_;
  bool active_ = true;
  std::optional<Command> &command_;
  MatchConfig match_config_;
  std::unique_ptr<Player> make_player(const Seat &seat, const Game &game);
  std::unique_ptr<Player> player_1_;
  std::unique_ptr<Player> player_2_;
  EngineInfoHistory engine_info_history_;
  void start(const MatchConfig &config);
  void update_player(Player &player);
  void undo();
  void redo();

public:
  MatchController(Game &game, MatchConfig &config,
                  std::optional<Command> &command)
      : game_(game), command_(command), match_config_(config) {
    start(config);
  };
  void update();
  const bool active() const { return active_; }
  const MatchConfig &match_config() const { return match_config_; }
  const bool player_is_human();
  const EngineInfoHistory &engine_info() { return engine_info_history_; }
  void set_active(bool active) { active_ = active; }
};
