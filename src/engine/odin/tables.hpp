#pragma once

#include <array>

#include "types.hpp"


namespace quarto {

inline constexpr std::array<Bitboard, 10> LINES = {
    0x000F, 0x00F0, 0x0F00, 0xF000,   // rows 0-3
    0x1111, 0x2222, 0x4444, 0x8888,   // cols 0-3
    0x8421, 0x1248                    // diagonals
};

// The lines through one cell: 2 normally, 3 on a diagonal. Fixed capacity so the
// whole table can be constexpr - a std::vector cannot outlive constant evaluation.
struct CellLines {
    std::array<Bitboard, 3> lines{};
    int count = 0;

    constexpr const Bitboard* begin() const { return lines.data(); }
    constexpr const Bitboard* end()   const { return lines.data() + count; }
};

constexpr std::array<CellLines, 16> makeCellLines()
{
    std::array<CellLines, 16> result{};

    for (int cell = 0; cell < 16; cell++) {
        for (Bitboard line : LINES) {
            if ((line >> cell) & 1) {
                result[cell].lines[result[cell].count++] = line;
            }
        }
    }

    return result;
}

inline constexpr std::array<CellLines, 16> CELL_LINES = makeCellLines();

// every run of the engine uses the identical table

// splitmix64: the standard seeding PRNG. One multiply-xorshift finalizer over a
// counter, which is enough here - these keys only need to be well spread, not
// cryptographic.
constexpr Key splitmix64(Key& state)
{
    state += 0x9E3779B97F4A7C15ULL;
    Key z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

struct ZobristKeys {
    std::array<std::array<Key, 16>, 16> cell{};   // [cell][piece]
    std::array<Key, 16>                 held{};   // piece in hand
};

constexpr ZobristKeys makeZobrist()
{
    ZobristKeys result{};
    Key state = 0x0DDC0FFEEBADF00DULL;   // arbitrary fixed seed

    for (auto& piece_keys : result.cell) {
        for (Key& key : piece_keys) {
            key = splitmix64(state);
        }
    }

    for (Key& key : result.held) {
        key = splitmix64(state);
    }

    return result;
}

// Sliced into the two names the rest of the engine uses. makeZobrist() runs twice
// during constant evaluation, but only the slices are stored.
inline constexpr std::array<std::array<Key, 16>, 16> ZOBRIST = makeZobrist().cell;
inline constexpr std::array<Key, 16>                 HELD    = makeZobrist().held;

}  // namespace quarto
