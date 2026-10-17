// engine_player.cpp - Implements EnginePlayer: builds SearchRequests from the
// current GameState.
#include "engine_player.hpp"
#include "game_state.hpp"
#include "process.hpp"
#include "types.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <mutex>
#include <optional>
#include <raylib.h>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace {
// The engine sits next to the GUI executable, wherever that was launched
// from. Passed as an argument list so a path with spaces stays intact.
std::vector<std::string> engine_command(const std::string &executable,
                                        const std::vector<std::string> &args) {
  std::vector<std::string> command{std::string(GetApplicationDirectory()) +
                                   executable};
  command.insert(command.end(), args.begin(), args.end());
  return command;
}
} // namespace

EnginePlayer::EnginePlayer(const Game &game, std::string name,
                           std::string executable,
                           std::vector<std::string> args, std::string go_args,
                           std::optional<std::uint64_t> seed)
    : game_(game), name_(name), go_args_(go_args), buffer_(""),
      engine_(
          engine_command(executable, args),
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
          nullptr, true) {
  if (seed)
    engine_.write("seed " + std::to_string(*seed) + "\n");
};

// Caller holds bestmove_mutex_.
void EnginePlayer::start_search() {
  // Drop anything left over from the previous search before starting a new
  // one, otherwise a stale cell gets played immediately.
  bestmove_ = {};
  // Read the top here, on the main thread, not when bestmove arrives on the
  // reader thread. A new search means a new move at this top, which ends the
  // redo line, so the infos stored from here on are outdated.
  search_top_ = game_.top();
  for (int k = search_top_; k < (int)engine_info_history_.size(); ++k)
    engine_info_history_[k].clear();
  engine_.write(format_engine_input());
  state_ = SearchState::Searching;
}

std::optional<Cell> EnginePlayer::poll_cell() {
  std::lock_guard<std::mutex> lock(bestmove_mutex_);
  if (state_ == SearchState::Idle)
    start_search();
  std::optional<Cell> c = bestmove_.cell;
  bestmove_.cell = std::nullopt;
  move_missing_ = state_ == SearchState::ResultReady && !c;
  return c;
};

std::optional<Piece> EnginePlayer::poll_piece() {

  std::lock_guard<std::mutex> lock(bestmove_mutex_);
  if (state_ == SearchState::Idle)
    start_search();
  std::optional<Piece> g = bestmove_.give;
  move_missing_ = state_ == SearchState::ResultReady && !g;
  if (g.has_value()) {
    // The search result is fully consumed once the piece is given.
    bestmove_ = {};
    state_ = SearchState::Idle;
  }
  return g;
};

void EnginePlayer::cancel() {
  std::lock_guard<std::mutex> lock(bestmove_mutex_);
  if (state_ == SearchState::Searching) {
    // The engine still answers a stopped search with a bestmove, for the old
    // position. Count it so the reader thread throws it away.
    engine_.write("stop\n");
    ++stale_bestmoves_;
  }
  bestmove_ = {};
  state_ = SearchState::Idle;
  move_missing_ = false;
}

void EnginePlayer::stop_search() {
  if (state_ == SearchState::Searching) {
    engine_.write("stop\n");
  }
}

std::array<std::vector<EngineInfo>, 17> EnginePlayer::engine_info() {
  std::lock_guard<std::mutex> lock(bestmove_mutex_);
  return engine_info_history_;
}

namespace {
// The token following key, if the line contains both.
std::optional<std::string> value_after(const std::vector<std::string> &tokens,
                                       const std::string &key) {
  auto it = std::find(tokens.begin(), tokens.end(), key);
  if (it == tokens.end() || it + 1 == tokens.end())
    return std::nullopt;
  return *(it + 1);
}

// Parses a decimal integer. At most 18 digits, so stoll cannot overflow.
std::optional<long long> parse_integer(const std::optional<std::string> &tok,
                                       bool allow_sign) {
  if (!tok)
    return std::nullopt;
  size_t start = (allow_sign && !tok->empty() && (*tok)[0] == '-') ? 1 : 0;
  bool is_number = tok->size() > start && tok->size() - start <= 18 &&
                   std::all_of(tok->begin() + start, tok->end(),
                               [](unsigned char c) { return std::isdigit(c); });
  if (!is_number)
    return std::nullopt;
  return std::stoll(*tok);
}
} // namespace

void EnginePlayer::parse_engine_output(std::string line) {

  std::vector<std::string> tokens;
  std::istringstream stream(line);
  std::string token;

  while (stream >> token) {
    tokens.push_back(token);
  }

  if (tokens.empty()) {
    return;
  }

  if (tokens[0] == "info") {
    // Only search progress carries a depth. Other infos (e.g. "info seed")
    // would otherwise be stored as a score-0 result and reset the evalbar.
    if (std::find(tokens.begin(), tokens.end(), "depth") == tokens.end())
      return;

    EngineInfo engine_output{};
    // Engines on Windows end their lines with "\r\n".
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    engine_output.raw = line;

    if (auto depth = parse_integer(value_after(tokens, "depth"), false))
      engine_output.depth = static_cast<int>(*depth);
    if (auto score = parse_integer(value_after(tokens, "score"), true))
      engine_output.score = static_cast<int16_t>(*score);
    if (auto mate = parse_integer(value_after(tokens, "mate"), true))
      engine_output.mate = static_cast<int>(*mate);
    if (auto nodes = parse_integer(value_after(tokens, "nodes"), false))
      engine_output.nodes = static_cast<std::uint64_t>(*nodes);
    engine_output.move = value_after(tokens, "move");

    std::lock_guard<std::mutex> lock(bestmove_mutex_);
    // Infos still belonging to a cancelled search are for the old position.
    if (stale_bestmoves_ > 0)
      return;
    engine_info_history_[search_top_].push_back(engine_output);

  } else if (tokens[0] == "bestmove" && tokens.size() >= 2) {
    if (tokens.size() > 1 &&
        std::regex_match(
            tokens[1], std::regex(R"(([a-dA-D][1-4]|-)\|([a-zA-Z]{4}|-|#))"))) {

      std::lock_guard<std::mutex> lock(bestmove_mutex_);

      if (stale_bestmoves_ > 0) {
        --stale_bestmoves_;
        return;
      }
      bestmove_ = parse_move(tokens[1]);
      state_ = SearchState::ResultReady;
    }
  }
};

OptionalMove EnginePlayer::parse_move(std::string move_string) {

  size_t pos = move_string.find('|');

  std::string cell_string = move_string.substr(0, pos);
  std::string hand_string = move_string.substr(pos + 1);
  OptionalMove move;

  // "-" means no placement / no give, so leave those fields empty.
  if (cell_string != "-") {
    int col = (std::tolower(static_cast<unsigned char>(cell_string[0])) - 'a');
    int row = 4 - (cell_string[1] - '0');
    move.cell = 4 * row + col;
  }
  if (hand_string != "-" && hand_string != "#") {
    Piece hand = 0;
    for (int i = 0; i < 4; ++i) {
      if (std::isupper(static_cast<unsigned char>(hand_string[i]))) {
        hand |= 1 << i;
      }
    }
    move.give = hand;
  }
  return move;
};

std::string EnginePlayer::format_position() {

  const std::array<char, 4> upper = {'C', 'H', 'S', 'F'};
  const std::array<char, 4> lower = {'c', 'h', 's', 'f'};
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
  std::string engine_input = "go" + go_args_;
  engine_input += " position " + format_position();
  engine_input += "\n";
  return engine_input;
};
