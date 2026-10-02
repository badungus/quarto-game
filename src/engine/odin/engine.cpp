#include "engine.hpp"
#include "position.hpp"
#include "search.hpp"
#include "types.hpp"
#include <bit>
#include <functional>
#include <thread>

namespace quarto {

void Engine::go(SearchCallbacks callback) {
  stop(); // only one search at a time
  if (search_thread_.joinable())
    search_thread_.join();
  search_.set_stop(false);

  int max_time = infinite_ ? DEFAULT_MOVETIME : movetime_;
  search_.set_movetime(max_time);
  int depth = std::min(depth_, std::popcount(pos_.empty_cells()));

  // The thread searches its own copy of the position, so the engine can be
  // reset below and reconfigured while the search is still running.
  search_thread_ = std::jthread([this, pos = pos_, depth, callback]() mutable {
    Result best_result = search_.search(pos, depth, callback.on_info);
    callback.on_bestmove(best_result);
  });

  set_position(DEFAULT_POSITION); // reset to start position
  depth_ = DEFAULT_DEPTH;
  movetime_ = DEFAULT_MOVETIME;
  infinite_ = DEFAULT_INFINITE;
};

void Engine::stop() { search_.set_stop(true); };

void Engine::set_position(
    std::tuple<Bitboard, std::array<Bitboard, 4>, Piece> pos) {
  auto [occ, prop, hand] = pos;
  pos_.load(occ, prop, hand);
}
} // namespace quarto
