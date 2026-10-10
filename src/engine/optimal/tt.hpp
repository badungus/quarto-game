#pragma once
#include "types.hpp"
#include <optional>
#include <vector>

namespace quarto {

class TranspositionTable {
private:
  // One stored position in 16 bytes: everything but the key's 64 bits packed
  // into `data` (layout in tt.cpp), and those bits XORed with `data`. A probe
  // undoes the XOR, so an entry whose two words come from different stores (a
  // write cut in half by another thread) fails the key check and is a miss.
  struct Entry {
    std::uint64_t check = 0; // key.bits ^ data
    std::uint64_t data = 0;
  };
  static_assert(sizeof(Entry) == 16);

  std::vector<Entry> table_;
  uint8_t gen_ = 0; // current search; wraps after 256, which is harmless

  std::size_t index(TTKey key) const;

public:
  // probe and store may be called by several threads at once. The rest may
  // not, and only while no thread is probing or storing.

  // Size in MB, like resize().
  explicit TranspositionTable(size_t mb) { resize(mb); }
  void clear();
  // Starts a new generation. Entries from earlier searches stay readable but
  // no longer block their slot: positions only gain pieces, so most of them
  // can never come up again.
  void new_search() { ++gen_; }
  // As many entries as fit in `mb` MB, at least one. Entries stored before a
  // resize land elsewhere after it: clear() the table.
  void resize(size_t mb);
  std::optional<TTHit> probe(TTKey key) const;
  // `key.extra` must fit in 20 bits and `depth` must be at least 1.
  void store(TTKey key, Score score, uint8_t depth, Move move, Bound bound);
};
} // namespace quarto
