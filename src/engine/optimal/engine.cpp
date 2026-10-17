#include "engine.hpp"
#include "canonical.hpp"
#include "position.hpp"
#include "solve.hpp"
#include "types.hpp"
#include <bit>
#include <functional>
#include <thread>

namespace quarto {

Engine::Engine(const std::string &book_path)
    : tt_(DEFAULT_TT_SIZE), book_(read_book(book_path)), solver_(tt_) {
  solver_.set_stop(&stop_);
  set_position(DEFAULT_POSITION);
}

void Engine::go(std::function<void(const Result &)> on_bestmove) {
  stop(); // only one solve at a time
  if (thread_.joinable())
    thread_.join();
  stop_ = false;

  // The thread works on its own copy of the position, so the engine can be
  // reset below and given the next position while it is still solving.
  thread_ = std::jthread([this, pos = pos_, on_bestmove]() mutable {
    on_bestmove(best_move(pos));
  });

  set_position(DEFAULT_POSITION); // reset to start position
}

Result Engine::best_move(Position &pos) {
  const int depth = std::popcount(pos.empty_cells());
  // The empty board with nothing in hand: every give is equivalent, and the
  // book has the position after it (piece 0 in hand) for the value.
  if (pos.occupied() == 0 && pos.in_hand() < 0) {
    pos.give_piece(0);
    Canon c = canonicalize(pos);
    pos.undo_give_piece();
    auto it = book_.find({c.cols, c.occ});
    // Without a book the value is unknown; Quarto is known to be a draw.
    const bool found = it != book_.end();
    const Score value = found ? Score(-it->second.value) : SOLVED_DRAW;
    return {value, {-1, 0}, depth, found, 0, mate_distance(value, pos)};
  }
  if (pos.in_hand() >= 0) {
    Canon c = canonicalize(pos);
    auto it = book_.find({c.cols, c.occ});
    int count = 0; // best moves listed for the position
    if (it != book_.end())
      for (Bitboard g : it->second.gives)
        count += std::popcount(g);
    if (count > 0) {
      // One of the best moves, at random, so games from the book vary.
      const auto &gives = it->second.gives;
      int pick = int(gen_() % unsigned(count));
      Move move = EMPTY_MOVE;
      for (int cell = 0; cell < 16 && move.cell < 0; ++cell) {
        const int n = std::popcount(gives[cell]);
        if (pick >= n) {
          pick -= n;
          continue;
        }
        Bitboard g = gives[cell];
        while (pick--)
          g &= g - 1;
        move = {Cell(cell), Piece(std::countr_zero(g))};
      }
      return {it->second.value, from_canonical(move, c), depth,
              true, 0, mate_distance(it->second.value, pos)};
    }
  }
  // Every TT entry is a bound on a proven score, so entries from earlier
  // moves stay valid; the new generation only lets them be replaced.
  tt_.new_search();
  SolveResult r = solver_.solve(pos);
  return {r.value,          r.move, depth, false,
          solver_.nodes(), mate_distance(r.value, pos)};
}

void Engine::set_position(
    std::tuple<Bitboard, std::array<Bitboard, 4>, Piece> pos) {
  auto [occ, prop, hand] = pos;
  pos_.load(occ, prop, hand);
}
} // namespace quarto
