#pragma once
#include "position.hpp"
#include "types.hpp"
#include <functional>
#include <vector>

namespace quarto {

// The engine's entire public surface: position in -> best move out.
//
//   occ            occupied cells
//   prop           per property bit, the cells whose piece has that bit set
//   piece_in_hand  the piece the side to move must place, or -1 at game start
//   depth          search depth (nullopt -> engine picks a default)
//
// Returns {cell, give}:
//   cell  0..15 board index (row * 4 + col) where the held piece is placed, or
//         -1 at game start (give-only move)
//   give  the piece handed to the opponent, or -1 if the game ends

struct Result {
  Score score;
  Move move;
};
struct SearchInfo {
  int depth;
  Result result;
};
using InfoFn = std::function<void(const SearchInfo &)>;

Move search(Position &pos, int depth, InfoFn cb);

std::vector<Result> negamax_root(Position &pos, int depth);

Score negamax(Position &, int depth, Score alpha, Score beta);

} // namespace quarto
