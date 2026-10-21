#pragma once
#include "position.hpp"
#include "solve.hpp"
#include "tt.hpp"
#include "types.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <thread>
#include <tuple>

namespace quarto {

const int DEFAULT_TT_SIZE = 2048;
const std::tuple<Bitboard, std::array<Bitboard, 4>, Piece> DEFAULT_POSITION = {
    0xffff, {0, 0, 0, 0}, -1};

// The engine's answer to go: the proven value for the side to move (a score
// as Solver::solve returns it) and a move that achieves it.
struct Result {
  Score value;
  Move move;
  int depth;           // placements left: the result covers all of them
  bool from_book;      // looked up, not solved
  std::uint64_t nodes; // nodes the solve visited, 0 from the book
  int mate;            // mate_distance of value, 0 for a draw
};

// Plays only proven moves, from the book if the position is in it, else from a
// solve to the end of the game. There are no depth or time limits.
class Engine {
private:
  Position pos_;
  TranspositionTable tt_;
  Book book_;
  std::mt19937_64 gen_{std::random_device{}()}; // picks among best book moves
  std::atomic<bool> stop_ = false;
  Solver solver_;       // declared after tt_ and stop_, which it refers to
  std::jthread thread_; // declared last: joins before the rest is gone

  Result best_move(Position &pos);

public:
  // Loads the book once; a missing file means solving every position.
  explicit Engine(const std::string &book_path);
  ~Engine() { stop(); }
  // Ends a running solve early. It still answers, with a meaningless result.
  void stop() { stop_ = true; }
  void set_position(std::tuple<Bitboard, std::array<Bitboard, 4>, Piece> pos);
  // Finds the move on its own thread and passes it to on_bestmove there.
  void go(std::function<void(const Result &)> on_bestmove);
};
} // namespace quarto
