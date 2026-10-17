// match_controller.hpp - Declares MatchController: owns GameState, History and
// two Players; applies Commands and drives turns.
#pragma once
#include "command.hpp"
#include "game_state.hpp"
#include "match_config.hpp"
#include "player.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

enum class GameStatus { PLAYER_ONE_WIN, DRAW, PLAYER_TWO_WIN };

struct GameResult {
  GameStatus status;
  std::string reason; // "win", "board full", or why an engine forfeited
};

class MatchController {
private:
  Game &game_;
  std::optional<GameResult> result_; // set once the game is over
  bool active_ = true;
  std::optional<Command> &command_;
  MatchConfig &match_config_;
  std::unique_ptr<Player> make_player(const Seat &seat, const Game &game,
                                      const std::string &go_args,
                                      std::optional<std::uint64_t> seed,
                                      bool squares);
  std::unique_ptr<Player> player_1_;
  std::unique_ptr<Player> player_2_;
  EngineInfoHistory engine_info_history_;
  void start(const MatchConfig &config);
  void update_player(Player &player, PlayerID id);
  void forfeit(PlayerID id, const std::string &reason);
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
  const std::optional<GameResult> &result() const { return result_; }
  const bool player_is_human();
  const EngineInfoHistory &engine_info() { return engine_info_history_; }
  void set_active(bool active) { active_ = active; }
};
