#pragma once
#include "engine.hpp"
#include "types.hpp"
#include <array>
#include <string>
#include <tuple>

namespace quarto {

std::tuple<Bitboard, std::array<Bitboard, 4>, Piece>
parse_position(const std::string &input);
std::string format_move(Move m, bool mate);
void uqi_loop(Engine &engine);

std::array<std::string, 16> create_tokens(std::string input);
} // namespace quarto
