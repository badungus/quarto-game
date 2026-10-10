#include "tt.hpp"
#include "types.hpp"
#include <algorithm>

namespace quarto {

void TranspositionTable::resize(size_t mb) {
  size_t bytes = mb * 1024ULL * 1024ULL;
  table_.resize(std::max(bytes / sizeof(Packed), size_t{1}));
}

void TranspositionTable::clear() {
  std::fill(table_.begin(), table_.end(), Packed{});
}

void TranspositionTable::store(TTKey key, Score score, uint8_t depth,
                               Move move, Bound bound) {
  Packed &e = table_[index(key)];
  const std::uint8_t old_depth = depth_of(e.data);
  const std::uint8_t old_gen = std::uint8_t(e.data >> GEN_SHIFT);
  if (old_depth == 0 || old_gen != gen_ || depth >= old_depth) {
    e = {key.bits,
         key.extra | std::uint64_t(std::uint16_t(score)) << SCORE_SHIFT |
             std::uint64_t(move.cell + 1) << CELL_SHIFT |
             std::uint64_t(move.give + 1) << GIVE_SHIFT |
             std::uint64_t(depth) << DEPTH_SHIFT |
             std::uint64_t(bound) << BOUND_SHIFT |
             std::uint64_t(gen_) << GEN_SHIFT};
  }
}
} // namespace quarto
