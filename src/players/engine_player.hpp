// engine_player.hpp - Declares EnginePlayer, which owns an IEngine instance and
// its chosen option values.
#pragma once

#include "../game/game_state.hpp"
#include "../game/types.hpp"
#include "player.hpp"
#include "process.hpp"
#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct OptionalMove {
  std::optional<Cell> cell;
  std::optional<Piece> give;
};

class EnginePlayer : public Player {
private:
  int depth_;
  const Game &game_;
  std::string name_;
  int movetime_;
  std::string buffer_;
  void parse_engine_output(std::string input);
  std::vector<EngineInfo> engine_info_;
  OptionalMove parse_move(std::string move_string);
  OptionalMove bestmove_;
  // bestmove_ and engine_info_ are written from the reader thread
  std::mutex bestmove_mutex_;
  TinyProcessLib::Process engine_;
  // Idle: no search for this turn yet. Searching: "go" sent, bestmove not yet
  // received. ResultReady: bestmove received, not fully played yet.
  enum class SearchState { Idle, Searching, ResultReady };
  SearchState state_ = SearchState::Idle;
  int stale_bestmoves_ = 0; // bestmoves still due from cancelled searches
  void start_search();
  std::string format_engine_input();
  std::string format_position();

public:
  EnginePlayer(const Game &game, std::string name, int depth, int movetime);
  ~EnginePlayer() { engine_.write("quit\n"); };
  std::optional<Cell> poll_cell() override;
  std::optional<Piece> poll_piece() override;
  void cancel() override;
  std::vector<EngineInfo> engine_info() override;
};
