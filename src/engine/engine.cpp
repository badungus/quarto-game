#include "engine.hpp"
#include "position.hpp"
#include "search.hpp"
#include "types.hpp"
#include <functional>

namespace quarto {

void Engine::go(SearchLimits limits, std::function<void(Move)> callback) {
  Move best_move = search(pos_, limits.depth);
};

void Engine::set_position(
    std::tuple<Bitboard, std::array<Bitboard, 4>, Piece> pos) {
  auto [occ, prop, hand] = pos;
  pos_.load(occ, prop, hand);
}
} // namespace quarto
