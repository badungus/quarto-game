#include "position.hpp"
#include "tables.hpp"

namespace quarto {

Position::Position(const Grid& grid,
                   std::optional<int> piece_in_hand,
                   const std::vector<int>& available)
{
    occ = 0;
    prop.fill(0);
    // No piece in hand at the opening -> -1, the same sentinel make/undo use.
    hand = static_cast<Piece>(piece_in_hand.value_or(-1));
    avail = 0;
    key = (hand >= 0) ? HELD[hand] : 0;
    top = 0;
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            if (!grid[row][col].has_value())
                continue;
            int piece = *grid[row][col];
            key ^= ZOBRIST[row * 4 + col][piece];
            occ |= (Bitboard)1 << (row * 4 + col);
            for (int i = 0; i < 4; ++i)
            {
                if ((piece >> i) & 1)
                {

                    prop[i] |= Bitboard{1} << (row * 4 + col);
                }
            }
        }
    }
    for (int i : available)
    {

        avail |= Bitboard{1} << i;
    }
}


void Position::place_piece(Cell c){
    history[top++] = {c, hand};
    if(c >= 0 && hand >= 0)
{
        key ^= ZOBRIST[c][hand];
        Bitboard mask = Bitboard{1} << c;
        occ |= mask;
        for (int i = 0; i < 4; ++i)
        {
            if ((hand >> i) & 1)
            {
                prop[i] |= mask;
            }
        }
}
}

void Position::give_piece(Piece p){
    if (hand >= 0)
    {
        key ^= HELD[hand];
    }
    if (p >= 0)
    {
        key ^= HELD[p];
        avail &= ~(Bitboard{1} << p);
    }
    hand = p;
}

void Position::undo_place_piece(){
    Undo u = history[--top];
    if (u.cell >= 0 && u.hand >= 0)
    {
        key ^= ZOBRIST[u.cell][u.hand];
        Bitboard mask = ~(Bitboard{1} << u.cell);
        occ &= mask;
        for (int i = 0; i < 4; ++i)
        {
            if ((u.hand >> i) & 1)
            {
                prop[i] &= mask;
            }
        }
    }
}

void Position::undo_give_piece(){
    if (hand >= 0)
    {
        key ^= HELD[hand];
        avail |= Bitboard{1} << hand;
    }
    // At the opening the root gives without placing first, so there is no
    // history entry to restore from - the hand simply goes back to empty.
    Piece prev = (top == 0) ? Piece(-1) : history[top-1].hand;
    if (prev >= 0)
    {
        key ^= HELD[prev];
    }
    hand = prev;
}
}  // namespace quarto
