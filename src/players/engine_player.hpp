// engine_player.hpp - Declares EnginePlayer, which owns an IEngine instance and
// its chosen option values.
#pragma once

#include "../game/game_state.hpp"
#include "../game/types.hpp"
#include "player.hpp"
#include <optional>
#include <string>

class EnginePlayer : public Player {
public:
  EnginePlayer(const Game &game, std::string name, int depth, int time);
  std::optional<Cell> poll_cell() override;
  std::optional<Piece> poll_piece() override;
};
