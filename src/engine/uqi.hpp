#pragma once
#include "engine.hpp"
#include "types.hpp"
#include <array>
#include <string>
#include <tuple>

namespace quarto {

enum class CommandType { uqi, is_ready, position, go, stop, quit, unknown };

struct Command {
  CommandType type;
  SearchLimits go;
  std::tuple<Bitboard, std::array<Bitboard, 4>, Piece> pos;
};

std::tuple<Bitboard, std::array<Bitboard, 4>, Piece>
parse_position(const std::string &input);
std::string format_move(Move m);
void uqi_loop(Engine &engine);
std::vector<Command> parse_command(const std::string &line);
SearchLimits parse_search_limits(const std::string &line);

std::array<std::string, 16> create_tokens(std::string input);
} // namespace quarto
