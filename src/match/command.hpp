// command.hpp - Command variant for everything that changes a match: SelectCell, SelectGive, Undo, Redo, JumpTo, NewGame, Pause, Resume, Step.
#pragma once
#include "../game/types.hpp"
#include "match_config.hpp"
#include <variant>

// Namespaced because types.hpp already defines a struct Undo.
namespace cmd {

// Human places the piece in hand on a board cell.
struct SelectCell {
  Cell cell;
};
// Human picks the piece the opponent must place next.
struct SelectGive {
  Piece piece;
};

// History navigation.
struct Undo {};
struct Redo {};
struct JumpTo {
  int ply; // number of half-moves from the start; 0 = empty board
};

// Match lifecycle.
struct NewGame {
  MatchConfig config;
};
struct Pause {};
struct Resume {};
struct Step {}; // while paused, let the engine make exactly one move

} // namespace cmd

using Command = std::variant<cmd::SelectCell, cmd::SelectGive, cmd::Undo,
                             cmd::Redo, cmd::JumpTo, cmd::NewGame, cmd::Pause,
                             cmd::Resume, cmd::Step>;
