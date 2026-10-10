#pragma once
#include "tables.hpp"
#include "types.hpp"
#include <optional>
#include <vector>

namespace quarto {

class TranspositionTable {
private:
  // One stored position in 16 bytes: the key's bits, then everything else
  // packed into one word. Cells and gives are stored plus one, so -1 fits.
  struct Packed {
    std::uint64_t bits = 0;
    std::uint64_t data = 0;
  };
  static constexpr int EXTRA_BITS = 20; // data bits 0-19
  static constexpr int SCORE_SHIFT = 20; // 16 bits
  static constexpr int CELL_SHIFT = 36;  // 5 bits
  static constexpr int GIVE_SHIFT = 41;  // 5 bits
  static constexpr int DEPTH_SHIFT = 46; // 8 bits
  static constexpr int BOUND_SHIFT = 54; // 2 bits
  static constexpr int GEN_SHIFT = 56;   // 8 bits
  static_assert(GEN_SHIFT + 8 == 64);
  static constexpr std::uint64_t EXTRA_MASK =
      (std::uint64_t{1} << EXTRA_BITS) - 1;

  // Every stored position has depth 1 or more, so depth 0 marks an empty
  // entry: no stored key needs a bit for it.
  static std::uint8_t depth_of(std::uint64_t data) {
    return std::uint8_t(data >> DEPTH_SHIFT);
  }

  static_assert(sizeof(Packed) == 16);

  std::vector<Packed> table_;
  uint8_t gen_ = 0; // current search; wraps after 256, which is harmless

  // The key's entry. The key is mixed so similar positions spread over the
  // table, and the mix is scaled to [0, size) by a multiply and shift, which
  // works for any table size.
  std::size_t index(TTKey key) const {
    Key state = key.bits;
    state = splitmix64(state) ^ key.extra;
    Key mix = splitmix64(state);
    return std::size_t((unsigned __int128)mix * table_.size() >> 64);
  }

public:
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
  std::optional<TTEntry> probe(TTKey key) const {
    const Packed &e = table_[index(key)];
    if (e.bits != key.bits || (e.data & EXTRA_MASK) != key.extra ||
        depth_of(e.data) == 0)
      return std::nullopt;
    return TTEntry{Score(std::uint16_t(e.data >> SCORE_SHIFT)),
                   depth_of(e.data),
                   {Cell(int(e.data >> CELL_SHIFT & 31) - 1),
                    Piece(int(e.data >> GIVE_SHIFT & 31) - 1)},
                   Bound(e.data >> BOUND_SHIFT & 3)};
  }
  // `key.extra` must fit in 20 bits and `depth` must be at least 1.
  void store(TTKey key, Score score, uint8_t depth, Move move, Bound bound);
};
} // namespace quarto
