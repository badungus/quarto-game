#include "tt.hpp"
#include "types.hpp"
#include <algorithm>
#include <bit>

namespace quarto {

void TranspositionTable::resize(size_t mb) {
  size_t bytes = mb * 1024ULL * 1024ULL;
  size_t entries = bytes / sizeof(TTEntry);

  // Use a power-of-two table size.
  entries = std::bit_floor(entries);

  table_.resize(entries);
  mask_ = entries - 1;
}

void TranspositionTable::clear() {
  std::fill(table_.begin(), table_.end(), TTEntry{});
}

void TranspositionTable::store(Key key, Score score, uint8_t depth, Move move,
                               Bound bound) {
  TTEntry &entry = table_[key & mask_];
  if (entry.key == 0 || entry.gen != gen_ || depth >= entry.depth) {
    entry = TTEntry{key, score, depth, move, bound, gen_};
  }
};
}; // namespace quarto
