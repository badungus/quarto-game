#pragma once

#include <cstdint>

namespace quarto {

using Bitboard = std::uint16_t; // one bit per cell, 0..15
using Key = std::uint64_t;      // 64-bit hash
using Piece = std::int8_t;      // 0..15 (4 property bits), -1 = none
using Cell = std::int8_t;       // 0..15 board index (row * 4 + col), -1 = none
using Score = std::int16_t;

struct Move {
  Cell cell;
  Piece give;
  bool operator==(const Move &) const = default;
}; // a Quarto move is ALWAYS this pair
struct Undo {
  Cell cell;
  Piece hand;
};

enum class Bound : uint8_t { Exact, Lower, Upper };

// position to compare in case multiple positions use the same tt index
struct TTKey {
  std::uint64_t bits;
  std::uint32_t extra;
};

// What the TT knows about a position.
struct TTHit {
  Score score;
  uint8_t depth;
  Move move;
  Bound bound;
};

constexpr Score WIN = 10000;
constexpr Score INF = 32000;
constexpr Move EMPTY_MOVE = {-1, -1};

} // namespace quarto
