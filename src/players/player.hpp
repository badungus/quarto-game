// player.hpp - Player interface shared by humans and engines: name, kind, and
// whether moves come from input or search.
#pragma once
#include "../game/types.hpp"
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
  virtual std::vector<EngineOutput> engine_output() { return {}; }
};
