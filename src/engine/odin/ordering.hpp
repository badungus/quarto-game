#pragma once
#include "position.hpp"
#include "types.hpp"
#include <array>

namespace quarto {

Cell best_cell(Bitboard &empty, std::array<Score, 16> &prior);
Piece best_give(Bitboard &avail, std::array<int, 16> &history);

std::array<Score, 16> compute_prior(const Position &pos);

}; // namespace quarto
