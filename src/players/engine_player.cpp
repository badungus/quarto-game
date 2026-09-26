// engine_player.cpp - Implements EnginePlayer: builds SearchRequests from the
// current GameState.
#include "engine_player.hpp"
#include "../game/game_state.hpp"
#include "../game/types.hpp"
#include <optional>
#include <string>

EnginePlayer::EnginePlayer(const Game &game, std::string name, int depth,
                           int time) {};

std::optional<Cell> EnginePlayer::poll_cell() { return std::nullopt; };
std::optional<Piece> EnginePlayer::poll_piece() { return std::nullopt; };
