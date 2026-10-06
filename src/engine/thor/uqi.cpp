#include "uqi.hpp"
#include "engine.hpp"
#include "search.hpp"
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

    // Output goes through osyncstream: the search thread prints too, and
    // lines from the two threads must not interleave.
    if (tokens[0] == "uqi") {
      std::osyncstream(std::cout) << "uqiok" << std::endl;
      continue;
    }

    if (tokens[0] == "isready") {
      std::osyncstream(std::cout) << "readyok" << std::endl;
      continue;
    }

    size_t depth_idx =
        std::find(tokens.begin(), tokens.end(), "depth") - tokens.begin();
    if (depth_idx < tokens.size()) {
      bool is_number =
          depth_idx + 1 < tokens.size() && !tokens[depth_idx + 1].empty() &&
          tokens[depth_idx + 1].size() <= 9 && // fits in an int for stoi
          std::all_of(tokens[depth_idx + 1].begin(),
                      tokens[depth_idx + 1].end(),
                      [](unsigned char c) { return std::isdigit(c); });
      if (is_number) {
        int depth = std::stoi(tokens[depth_idx + 1]);
        engine.set_depth(depth);
      }
    }

    size_t movetime_idx =
        std::find(tokens.begin(), tokens.end(), "movetime") - tokens.begin();
    if (movetime_idx < tokens.size()) {
      bool is_number =
          movetime_idx + 1 < tokens.size() &&
          !tokens[movetime_idx + 1].empty() &&
          tokens[movetime_idx + 1].size() <= 9 && // fits in an int for stoi
          std::all_of(tokens[movetime_idx + 1].begin(),
                      tokens[movetime_idx + 1].end(),
                      [](unsigned char c) { return std::isdigit(c); });
      if (is_number) {
        int movetime = std::stoi(tokens[movetime_idx + 1]);
        engine.set_movetime(movetime);
      }
    }

    // "seed N" fixes the tie-break generator; it stays in effect for every
    // following search until the next seed.
    size_t seed_idx =
        std::find(tokens.begin(), tokens.end(), "seed") - tokens.begin();
    if (seed_idx < tokens.size()) {
      bool is_number =
          seed_idx + 1 < tokens.size() && !tokens[seed_idx + 1].empty() &&
          tokens[seed_idx + 1].size() <= 19 && // fits in a uint64 for stoull
          std::all_of(tokens[seed_idx + 1].begin(), tokens[seed_idx + 1].end(),
                      [](unsigned char c) { return std::isdigit(c); });
      if (is_number) {
        engine.set_seed(std::stoull(tokens[seed_idx + 1]));
      }
    }

    if (std::find(tokens.begin(), tokens.end(), "infinite") != tokens.end()) {
      engine.set_infinite(true);
    }

    size_t pos_idx =
        std::find(tokens.begin(), tokens.end(), "position") - tokens.begin();
    if (pos_idx + 1 <
        tokens.size()) { // "position" must be followed by a string

      if (std::regex_match(
              tokens[pos_idx + 1],
              std::regex(
                  R"((?:([a-zA-Z]{4}|\.){4}\/){3}(?:([a-zA-Z]{4}|\.){4})\|([a-zA-Z]{4}|\-))"))) {
        // check if string matches position format
        engine.set_position(parse_position(tokens[pos_idx + 1]));
      }
    }
    if (tokens[0] == "go") {
      engine.go({.on_info =
                     [](const SearchInfo &i) {
                       std::osyncstream(std::cout)
                           << "info depth " << i.depth << " score "
                           << i.result.score
                           << (i.mate != 0 ? " mate " + std::to_string(i.mate)
                                           : "")
                           << " nodes " << i.nodes << " move "
                           << format_move(i.result.move,
                                          (i.result.score == WIN))
                           << std::endl;
                     },
                 .on_bestmove =
                     [](const Result result) {
                       std::osyncstream(std::cout)
                           << "bestmove "
                           << format_move(result.move, (result.score == WIN))
                           << std::endl;
                     },
                 .on_seed =
                     [](std::uint64_t seed) {
                       std::osyncstream(std::cout)
                           << "info seed " << seed << std::endl;
                     }});
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

// default: ..../..../..../....|-
// example: ScHfschf../ScHF..scHF/..../...schF|-
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
