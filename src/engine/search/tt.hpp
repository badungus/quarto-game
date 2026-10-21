#pragma once
#include "types.hpp"
#include <optional>
#include <vector>

namespace quarto {

class TranspositionTable {
private:
  // Entry has 16 bytes, using XOR check, so the key check misses if it has been
  // cut in half from another thread
  struct Entry {
    std::uint64_t check = 0; // key.bits ^ data
    std::uint64_t data = 0;
  };
  static_assert(sizeof(Entry) == 16);

  std::vector<Entry> table_;
  uint8_t gen_ = 0; // current search; wraps after 256, which is harmless

  std::size_t index(TTKey key) const;

public:
  // probe and store may be called by several threads at once
  explicit TranspositionTable(size_t mb) { resize(mb); }
  void clear();
  void new_search() { ++gen_; }
  void resize(size_t mb);
  std::optional<TTHit> probe(TTKey key) const;
  void store(TTKey key, Score score, uint8_t depth, Move move, Bound bound);
};
} // namespace quarto
