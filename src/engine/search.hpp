#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "types.hpp"   // Grid
#include "wincheck.hpp"

namespace quarto {

// The engine's entire public surface: position in -> best move out.
//
//   grid           current board (4x4)
//   piece_in_hand  the piece the side to move must place (or nullopt at game start)
//   available      pieces not yet placed and not in hand
//   depth          search depth (nullopt -> engine picks a default)
//
// Returns {cell, give}:
//   cell  0..15 board index (row * 4 + col) where the held piece is placed, or -1
//   give  the piece handed to the opponent, or -1 if none remain
std::pair<std::pair<int, int>, int> find_best_move(const Grid& grid,
                                   std::optional<int> piece_in_hand,
                                   const std::vector<int>& available,
                                   std::optional<int> depth);

Score negamax(Position&, int depth, Score alpha, Score beta);
                                   
}  // namespace quarto
