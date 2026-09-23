#pragma once

#include <array>
#include <vector>
#include <optional>

#include "types.hpp"

namespace quarto {

class Position {
    Bitboard occ;                   // occupied cells
    std::array<Bitboard,4> prop;    // cells containing a property
    Bitboard avail;                 // available pieces, neither placed nor in hand
    Piece hand;                     // piece the side to move has to play
    Key key;                        // zobrist
    std::array<Undo, 17> history;   // move history
    int top;

    // Read-only access for the standalone unit test (tests/test_position.cpp).
    // Grants no mutation and costs nothing at runtime.
    friend struct PositionProbe;

    public:
    // piece_in_hand is optional because at the opening no piece has been handed
    // over yet; nullopt maps to the NO_PIECE (-1) sentinel used throughout.
    // Both containers are taken by const reference - this runs once per search,
    // but there is no reason to copy them.
    Position(const Grid& grid,
             std::optional<int> piece_in_hand,
             const std::vector<int>& available);
    void place_piece(Cell c);
    void give_piece(Piece p);
    void undo_place_piece();
    void undo_give_piece();

    Bitboard empty_cells() const { return Bitboard(~occ & 0xFFFF); }
    Bitboard available()   const { return avail; }
    Bitboard occupied()    const { return occ; }
    Piece    in_hand()     const { return hand; }
    Key      hash()        const { return key; }
    bool     is_full()     const { return occ == 0xFFFF; }

    const std::array<Bitboard, 4>& props() const { return prop; }
};

}  // namespace quarto
