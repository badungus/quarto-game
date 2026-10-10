#pragma once
#include "engine.hpp"
#include "types.hpp"
#include <array>
#include <string>
#include <tuple>

namespace quarto {

// True if `input` is in the position format, e.g. ChSfchsf../ChSF..chSF/..../...chsF|-;
// parse_position expects one that is.
bool valid_position(const std::string &input);
std::tuple<Bitboard, std::array<Bitboard, 4>, Piece>
parse_position(const std::string &input);
std::string format_move(Move m, bool mate);
void uqi_loop(Engine &engine);

std::array<std::string, 16> create_tokens(std::string input);
} // namespace quarto
