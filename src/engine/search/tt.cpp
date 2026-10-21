#include "tt.hpp"
#include "tables.hpp"
#include "types.hpp"
#include <algorithm>
#include <atomic>

namespace quarto {

namespace {
// Layout of Entry::data. Cells and gives are stored plus one, so -1 fits.
constexpr int EXTRA_BITS = 20;  // bits 0-19: key.extra
constexpr int SCORE_SHIFT = 20; // 16 bits
constexpr int CELL_SHIFT = 36;  // 5 bits
constexpr int GIVE_SHIFT = 41;  // 5 bits
constexpr int DEPTH_SHIFT = 46; // 8 bits
constexpr int BOUND_SHIFT = 54; // 2 bits
constexpr int GEN_SHIFT = 56;   // 8 bits
static_assert(GEN_SHIFT + 8 == 64);
constexpr std::uint64_t EXTRA_MASK = (std::uint64_t{1} << EXTRA_BITS) - 1;

// Every stored position has depth 1 or more, so depth 0 marks an empty entry:
// no stored key needs a bit for it.
std::uint8_t depth_of(std::uint64_t data) {
  return std::uint8_t(data >> DEPTH_SHIFT);
}

// Entry words are read and written by several threads at once. Relaxed
// atomics make that defined behaviour and cost nothing on x86, the XOR check
// catches an entry whose two words come from different stores.
std::uint64_t load(const std::uint64_t &word) {
  return std::atomic_ref(const_cast<std::uint64_t &>(word))
      .load(std::memory_order_relaxed);
}
void save(std::uint64_t &word, std::uint64_t value) {
  std::atomic_ref(word).store(value, std::memory_order_relaxed);
}
} // namespace

std::size_t TranspositionTable::index(TTKey key) const {
  Key state = key.bits;
  state = splitmix64(state) ^ key.extra;
  Key mix = splitmix64(state);
  return std::size_t((unsigned __int128)mix * table_.size() >> 64);
}

void TranspositionTable::resize(size_t mb) {
  size_t bytes = mb * 1024ULL * 1024ULL;
  table_.resize(std::max(bytes / sizeof(Entry), size_t{1}));
}

void TranspositionTable::clear() {
  std::fill(table_.begin(), table_.end(), Entry{});
}

std::optional<TTHit> TranspositionTable::probe(TTKey key) const {
  const Entry &entry = table_[index(key)];
  // Each word is read once, so the checks below and the result agree.
  const std::uint64_t data = load(entry.data);
  const std::uint64_t bits = load(entry.check) ^ data;
  if (bits != key.bits || (data & EXTRA_MASK) != key.extra ||
      depth_of(data) == 0)
    return std::nullopt;
  return TTHit{Score(std::uint16_t(data >> SCORE_SHIFT)),
               depth_of(data),
               {Cell(int(data >> CELL_SHIFT & 31) - 1),
                Piece(int(data >> GIVE_SHIFT & 31) - 1)},
               Bound(data >> BOUND_SHIFT & 3)};
}

void TranspositionTable::store(TTKey key, Score score, uint8_t depth, Move move,
                               Bound bound) {
  Entry &entry = table_[index(key)];
  const std::uint64_t old = load(entry.data);
  const std::uint8_t old_depth = depth_of(old);
  const std::uint8_t old_gen = std::uint8_t(old >> GEN_SHIFT);
  if (old_depth == 0 || old_gen != gen_ || depth >= old_depth) {
    const std::uint64_t data =
        key.extra | std::uint64_t(std::uint16_t(score)) << SCORE_SHIFT |
        std::uint64_t(move.cell + 1) << CELL_SHIFT |
        std::uint64_t(move.give + 1) << GIVE_SHIFT |
        std::uint64_t(depth) << DEPTH_SHIFT |
        std::uint64_t(bound) << BOUND_SHIFT | std::uint64_t(gen_) << GEN_SHIFT;
    save(entry.check, key.bits ^ data);
    save(entry.data, data);
  }
}
} // namespace quarto
