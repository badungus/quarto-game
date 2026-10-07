#pragma once
#include "tables.hpp"
#include "types.hpp"
#include <vector>

namespace quarto {

class TranspositionTable {
private:
  std::vector<TTEntry> table_;
  Key mask_;
  uint8_t gen_ = 0; // current search; wraps after 256, which is harmless

public:
  // Size in MB, like resize().
  explicit TranspositionTable(size_t mb) { resize(mb); }
  void clear();
  // Starts a new generation. Entries from earlier searches stay readable but
  // no longer block their slot: positions only gain pieces, so most of them
  // can never come up again.
  void new_search() { ++gen_; }
  void resize(size_t mb);
  const TTEntry *probe(Key key) const {
    const TTEntry &e = table_[key & mask_];
    return e.key == key ? &e : nullptr;
  }
  void store(Key key, Score score, uint8_t depth, Move move, Bound bound);
};
} // namespace quarto
