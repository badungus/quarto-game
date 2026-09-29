#include "engine.hpp"
#include "position.hpp"
#include "search.hpp"
#include "types.hpp"
#include <bit>
#include <functional>

namespace quarto {

void Engine::go(SearchCallbacks callback) {
  int max_time = infinite_ ? DEFAULT_MOVETIME : movetime_;
  search_.set_movetime(max_time);
  depth_ = std::min(depth_, std::popcount(pos_.empty_cells()));
  Move best_move = search_.search(pos_, depth_, callback.on_info);
  callback.on_bestmove(best_move);
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
