// engine_player.cpp - Implements EnginePlayer: builds SearchRequests from the
// current GameState.
#include "engine_player.hpp"
#include "../game/game_state.hpp"
#include "../game/types.hpp"
#include "process.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <raylib.h>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

EnginePlayer::EnginePlayer(const Game &game, std::string name, int depth,
                           int movetime)
    : game_(game), name_(name), depth_(depth), movetime_(movetime), buffer_(""),
      // The engine sits next to the GUI executable, wherever that was launched
      // from. Passed as an argument list so a path with spaces stays intact.
      engine_(
          std::vector<std::string>{std::string(GetApplicationDirectory()) +
                                   "quarto-engine"},
          "",
          [&](const char *bytes, size_t n) {
            buffer_.append(bytes, n);

            size_t pos;
            while ((pos = buffer_.find('\n')) != std::string::npos) {
              std::string line = buffer_.substr(0, pos);

              // Remove '\n'
              buffer_.erase(0, pos + 1);

              parse_engine_output(line);
            }
          },
          nullptr, true) {};

std::optional<Cell> EnginePlayer::poll_cell() {
  if (!searching_) {
    engine_.write(format_engine_input());
    searching_ = true;
  }
  std::optional<Cell> c = bestmove_.cell;
  if (bestmove_.cell != std::nullopt) {
    bestmove_.cell = std::nullopt;
  }
  return c;
};

std::optional<Cell> EnginePlayer::poll_piece() {

  if (!searching_) {
    engine_.write(format_engine_input());
    searching_ = true;
  }
  std::optional<Cell> g = bestmove_.give;
  if (bestmove_.give != std::nullopt) {
    bestmove_.give = std::nullopt;
    searching_ = false;
  }
  return g;
};

void EnginePlayer::parse_engine_output(std::string line) {

  std::vector<std::string> tokens;
  std::istringstream stream(line);
  std::string token;

  while (stream >> token) {
    tokens.push_back(token);
  }

  if (tokens[0] == "info") {

    EngineOutput engine_output{};

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
        engine_output.depth = depth;
      }
    }

    size_t score_idx =
        std::find(tokens.begin(), tokens.end(), "score") - tokens.begin();
    if (score_idx < tokens.size()) {
      bool is_number =
          score_idx + 1 < tokens.size() && !tokens[score_idx + 1].empty() &&
          tokens[score_idx + 1].size() <= 9 && // fits in an int for stoi
          std::all_of(tokens[score_idx + 1].begin(),
                      tokens[score_idx + 1].end(),
                      [](unsigned char c) { return std::isdigit(c); });
      if (is_number) {
        uint32_t score = std::stoi(tokens[depth_idx + 1]);
        engine_output.score = score;
      }

      size_t move_idx =
          std::find(tokens.begin(), tokens.end(), "move") - tokens.begin();
      if (move_idx < tokens.size()) {
        engine_output.move = tokens[move_idx + 1];
      };
      engine_output_.push_back(engine_output);
    }
  };

  if (tokens[0] == "bestmove") {

    if (std::regex_match(
            tokens[1], std::regex(R"(([a-dA-D][1-4]|-)\|([a-zA-Z]{4}|-|#))"))) {
      bestmove_ = parse_move(tokens[1]);
    }
  };
};

OptionalMove EnginePlayer::parse_move(std::string move_string) {

  size_t pos = move_string.find('|');

  std::string cell_string = move_string.substr(0, pos);
  std::string hand_string = move_string.substr(pos + 1);
  Cell cell_idx;
  Piece hand = 0;

  if (cell_string == "-") {
    cell_idx = -1;
  } else {
    int col = (cell_string[0] - 'a');
    int row = 4 - (cell_string[1] - '0');
    cell_idx = 4 * row + col;
  }
  if (hand_string == "-" || hand_string == "#") {
    hand = -1;
  } else {
    for (int i = 0; i < 4; ++i) {
      if (std::isupper(static_cast<unsigned char>(hand_string[i]))) {
        hand |= 1 << i;
      }
    }
  }
  return {cell_idx, hand};
};

std::string EnginePlayer::format_position() {

  const std::array<char, 4> upper = {'S', 'C', 'H', 'F'};
  const std::array<char, 4> lower = {'s', 'c', 'h', 'f'};
  std::string position = "";
  int cell_counter = 0;
  for (const Piece &p : game_.board()) {
    if (p < 0) {
      position += '.';
      cell_counter++;
    } else {

      for (int i = 0; i < 4; ++i) {
        position += (p & (1 << i)) ? upper[i] : lower[i];
      }
      cell_counter++;
    }
    if (cell_counter % 4 == 0 && cell_counter < 16) {
      position += '/';
    }
  }
  position += '|';
  if (game_.hand() < 0) {
    position += '-';
  } else {
    for (int i = 0; i < 4; ++i) {
      position += (game_.hand() & (1 << i)) ? upper[i] : lower[i];
    }
  }
  return position;
};

std::string EnginePlayer::format_engine_input() {
  std::string engine_input = "go";
  engine_input += " depth " + std::to_string(depth_);
  engine_input += " movetime " + std::to_string(movetime_);
  engine_input += " position " + format_position();
  engine_input += "\n";
  return engine_input;
};
