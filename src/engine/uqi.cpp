#include "uqi.hpp"
#include "engine.hpp"
#include "types.hpp"
#include <array>
#include <cctype>
#include <iostream>
#include <string>
#include <tuple>

namespace quarto {

Command parse_command(const std::string &line) {
  if (line == "uqi")
    return {CommandType::uqi};

  if (line == "isready")
    return {CommandType::is_ready};

  if (line.starts_with("position"))
    return {CommandType::position, {}, {parse_position(line.substr(8))}};

  if (line.starts_with("go"))
    return {CommandType::go, {}};

  if (line == "stop")
    return {CommandType::stop};

  if (line == "quit")
    return {CommandType::quit};

  return {CommandType::unknown};
}

void uqi_loop(Engine &engine) {
  std::string line;
  while (std::getline(std::cin, line)) {
    Command cmd = parse_command(line);
    switch (cmd.type) {
    case CommandType::uqi:
      std::cout << "uqiok" << std::endl;
      break;
    case CommandType::is_ready:
      std::cout << "readyok" << std::endl;
      break;
    case CommandType::position:
      engine.set_position(cmd.pos);
      break;
    case CommandType::go:
      engine.go(cmd.go, [](Move m) {
        std::cout << "bestmove " << format_move(m) << std::endl;
      });
      break;
    case CommandType::stop:
      engine.stop();
      break;
    case CommandType::quit:
      engine.stop();
    case CommandType::unknown:
      break;
    }
  }
  engine.stop();
}

// default: ..../..../..../....|-
// example: ScHfschf../ScHf..scHF/..../...schF|-
std::tuple<Bitboard, std::array<Bitboard, 4>, Piece>
parse_position(const std::string &input) {
  Bitboard occ = 0;
  std::array<Bitboard, 4> board;
  board.fill(0);
  Piece hand = 0;

  size_t pos = input.find('|');

  std::string board_string = input.substr(0, pos);
  std::string hand_string = input.substr(pos + 1);
  if (hand_string == "-") {
    hand = -1;
  } else {
    for (int i = 0; i < 4; ++i) {
      if (std::isupper(static_cast<unsigned char>(hand_string[i]))) {
        hand |= 1 << i;
      }
    }
  }

  std::array<std::string, 16> squares = create_tokens(board_string);
  int idx = 0;
  for (const std::string &piece : squares) {
    if (piece != ".") {
      occ |= 1 << idx;
      for (int i = 0; i < 4; ++i) {
        if (std::isupper(static_cast<unsigned char>(piece[i]))) {
          board[i] |= 1 << idx;
        }
      }
    }
    ++idx;
  }
  return {occ, board, hand};
}

std::string format_move(Move m) {
  std::string move_string = "";
  if (m.cell < 0) {
    move_string += '-';
  } else {
    // Cell rows run top to bottom, ranks bottom to top: cell 12 is a1.
    int col = m.cell % 4;
    int rank = 4 - m.cell / 4;
    move_string += static_cast<char>('a' + col);
    move_string += std::to_string(rank);
  }
  move_string += '|';

  if (m.give < 0) {
    move_string += '-';
  } else {
    const std::array<char, 4> upper = {'S', 'C', 'H', 'F'};
    const std::array<char, 4> lower = {'s', 'c', 'h', 'f'};

    for (int i = 0; i < 4; ++i) {
      move_string += (m.give & (1 << i)) ? upper[i] : lower[i];
    }
  }

  return move_string;
}

std::array<std::string, 16> create_tokens(std::string input) {
  std::array<std::string, 16> squares;
  int idx = 0;
  for (char c : input) {
    if (c == '/') {
      continue;
    }
    if (c == '.') {
      squares[idx++] += c;
    } else {
      squares[idx] += c;
      if (std::tolower(c) == 'f') {
        idx++;
      }
    }
  }
  return squares;
}
} // namespace quarto
