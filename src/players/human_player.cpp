// human_player.cpp - Implements HumanPlayer.
#include "human_player.hpp"
#include "../game/types.hpp"
#include <optional>

std::optional<Cell> HumanPlayer::poll_cell() {
  if (!command_) {
    return std::nullopt;
  }
  if (auto *c = std::get_if<cmd::SelectCell>(&*command_)) {
    return c->cell;
  }

  return std::nullopt;
};
std::optional<Piece> HumanPlayer::poll_piece() {

  if (!command_) {
    return std::nullopt;
  }

  if (auto *c = std::get_if<cmd::SelectGive>(&*command_)) {
    return c->piece;
  }

  return std::nullopt;
};
