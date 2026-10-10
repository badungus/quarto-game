#include "uqi.hpp"
#include "engine.hpp"
#include "solve.hpp"
#include "types.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <syncstream>
#include <tuple>

namespace quarto {

void uqi_loop(Engine &engine) {
  std::string line;

  while (std::getline(std::cin, line)) {
    std::vector<std::string> tokens;
    std::istringstream stream(line);
    std::string token;

    while (stream >> token) {
      tokens.push_back(token);
    }

    if (tokens.empty())
      continue; // no valid input

    // Output goes through osyncstream: the solve thread prints too, and
    // lines from the two threads must not interleave.
    if (tokens[0] == "uqi") {
      std::osyncstream(std::cout) << "uqiok" << std::endl;
      continue;
    }

    if (tokens[0] == "isready") {
      std::osyncstream(std::cout) << "readyok" << std::endl;
      continue;
    }

    // There are no search options: depth, movetime and the like are ignored,
    // since the engine always plays a proven move.
    size_t pos_idx =
        std::find(tokens.begin(), tokens.end(), "position") - tokens.begin();
    // "position" must be followed by a string in the position format.
    if (pos_idx + 1 < tokens.size() && valid_position(tokens[pos_idx + 1]))
      engine.set_position(parse_position(tokens[pos_idx + 1]));
    if (tokens[0] == "go") {
      engine.go([](const Result &r) {
        // One info line, so the GUI shows the result: the depth is the rest
        // of the game, the score WIN, 0 or -WIN for a win, draw or loss.
        const bool win = r.value == SOLVED_WIN;
        std::osyncstream(std::cout)
            << "info depth " << r.depth << " score " << r.value * WIN
            << " nodes " << r.nodes << " move " << format_move(r.move, win)
            << (r.from_book ? " book" : "") << "\n"
            << "bestmove " << format_move(r.move, win) << std::endl;
      });
    }

    if (tokens[0] == "stop") {
      engine.stop();
    }

    if (tokens[0] == "quit") {
      engine.stop();
      break;
    }
  }
}

bool valid_position(const std::string &input) {
  static const std::regex format(
      R"((?:([a-zA-Z]{4}|\.){4}\/){3}(?:([a-zA-Z]{4}|\.){4})\|([a-zA-Z]{4}|\-))");
  return std::regex_match(input, format);
}

// default: ..../..../..../....|-
// example: ChSfchsf../ChSF..chSF/..../...chsF|-
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

std::string format_move(Move m, bool mate) {
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
    move_string += mate ? '#' : '-';
  } else {
    const std::array<char, 4> upper = {'C', 'H', 'S', 'F'};
    const std::array<char, 4> lower = {'c', 'h', 's', 'f'};

    for (int i = 0; i < 4; ++i) {
      move_string += (m.give & (1 << i)) ? upper[i] : lower[i];
    }
  }
  return move_string;
}

std::array<std::string, 16> create_tokens(std::string input) {
  std::array<std::string, 16> squares;
  int idx = 0;
  int letter_counter = 0;
  for (char c : input) {
    if (c == '/') {
      continue;
    }
    if (c == '.') {
      squares[idx++] += c;
    } else {
      squares[idx] += c;
      if (++letter_counter == 4) {
        idx++;
        letter_counter = 0;
      }
    }
  }
  return squares;
}
} // namespace quarto
