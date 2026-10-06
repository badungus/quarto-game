// player.hpp - Player interface shared by humans and engines: name, kind, and
// whether moves come from input or search.
#pragma once
#include "types.hpp"
#include <array>
#include <optional>
#include <vector>

class Player {
private:
public:
  Player() = default;
  virtual ~Player() = default;
  virtual std::optional<Cell> poll_cell() { return std::nullopt; }
  virtual std::optional<Piece> poll_piece() { return std::nullopt; }
  // Drop any move in progress, e.g. because the position was undone.
  virtual void cancel() {}
  virtual std::array<std::vector<EngineInfo>, 17> engine_info() { return {}; }
  virtual void stop_search() { return; }
  // True when the last poll found the finished answer without the part that
  // was asked for, so that part will never come.
  virtual bool move_missing() const { return false; }
};
