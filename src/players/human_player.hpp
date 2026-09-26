// human_player.hpp - Declares HumanPlayer, whose moves are supplied by UI input
// through the MatchController.
#pragma once
#include "../game/types.hpp"
#include "player.hpp"
#include <optional>

class HumanPlayer : public Player {
public:
  std::optional<Cell> poll_cell() override;
  std::optional<Piece> poll_piece() override;
};
