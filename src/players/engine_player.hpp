// engine_player.hpp - Declares EnginePlayer, which owns an IEngine instance and
// its chosen option values.
#pragma once

#include "game_state.hpp"
#include "player.hpp"
#include "process.hpp"
#include "types.hpp"
#include <array>
#include <cstdint>
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
  const Game &game_;
  std::string name_;
  std::string go_args_; // search limits, appended to "go"
  std::string buffer_;
  void parse_engine_output(std::string input);
  // engine_info_history_[k] holds the info of the search made at game top k.
  std::array<std::vector<EngineInfo>, 17> engine_info_history_;
  int search_top_ = 0; // game top the running search belongs to
  OptionalMove parse_move(std::string move_string);
  OptionalMove bestmove_;
  // bestmove_ and are written from the reader thread
  std::mutex bestmove_mutex_;
  TinyProcessLib::Process engine_;
  // Idle: no search for this turn yet. Searching: "go" sent, bestmove not yet
  // received. ResultReady: bestmove received, not fully played yet.
  enum class SearchState { Idle, Searching, ResultReady };
  SearchState state_ = SearchState::Idle;
  int stale_bestmoves_ = 0; // bestmoves still due from cancelled searches
  bool move_missing_ = false;
  void start_search();
  std::string format_engine_input();
  std::string format_position();

public:
  EnginePlayer(const Game &game, std::string name, std::string executable,
               std::string go_args, std::optional<std::uint64_t> seed);
  ~EnginePlayer() { engine_.write("quit\n"); };
  std::optional<Cell> poll_cell() override;
  std::optional<Piece> poll_piece() override;
  void cancel() override;
  std::array<std::vector<EngineInfo>, 17> engine_info() override;
  void stop_search() override;
  bool move_missing() const override { return move_missing_; }
};
