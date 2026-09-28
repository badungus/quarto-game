#include "engine.hpp"
#include "position.hpp"
#include "search.hpp"
#include "types.hpp"
#include <functional>

namespace quarto {

void Engine::go(SearchCallbacks callback) {
  Move best_move = search(pos_, depth_, callback.on_info);
  callback.on_bestmove(best_move);
  set_position(STARTING_POSITION); // reset to start position
  depth_ = DEFAULT_DEPTH;
  movetime_ = DEFAULT_MOVETIME;
  infinite_ = DEFAULT_INFINITE;
};

void Engine::stop() {};

void Engine::set_position(
    std::tuple<Bitboard, std::array<Bitboard, 4>, Piece> pos) {
  auto [occ, prop, hand] = pos;
  pos_.load(occ, prop, hand);
}
} // namespace quarto
