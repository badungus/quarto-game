#pragma once
#include "position.hpp"
#include "types.hpp"
#include <array>

namespace quarto {

Cell best_cell(Bitboard &empty, std::array<std::array<int, 16>, 16> &history,
               const Piece &hand);
Piece best_give(Bitboard &avail, std::array<int, 16> &history);

}; // namespace quarto
