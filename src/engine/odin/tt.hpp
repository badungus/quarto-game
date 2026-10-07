#pragma once
#include "tables.hpp"
#include "types.hpp"
#include <vector>

namespace quarto {

class TranspositionTable {
private:
  std::vector<TTEntry> table_;
  Key mask_;

public:
  // Size in MB, like resize().
  explicit TranspositionTable(size_t mb) { resize(mb); }
  void clear();
  void resize(size_t mb);
  const TTEntry *probe(Key key) const {
    const TTEntry &e = table_[key & mask_];
    return e.key == key ? &e : nullptr;
  }
  void store(Key key, Score score, uint8_t depth, Move move, Bound bound);
};
} // namespace quarto
