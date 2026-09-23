#include "search.hpp"

#include <algorithm>
#include <bit>
#include <random>

namespace quarto {

// Horizon evaluation: the side to move wins outright if any placement of the
// held piece completes a line; otherwise guess a draw. Restores the position.
static Score eval(Position& pos) {
    Bitboard empty = pos.empty_cells();
    while (empty) {
        Cell cell = std::countr_zero(empty);
        pos.place_piece(cell);
        bool win = move_wins(pos, cell);
        pos.undo_place_piece();
        if (win) return WIN;
        empty &= (empty - 1);
    }
    return 0;
}

inline std::pair<int, int> index_to_position(int index){
    if(index == -1) return {-1, -1};
    return {index /4, index % 4};
}

std::pair<std::pair<int, int>, int> find_best_move(const Grid& grid,
                                   std::optional<int> piece_in_hand,
                                   const std::vector<int>& available,
                                   std::optional<int> depth) {
    Position pos(grid, piece_in_hand, available); // create position object
    Bitboard empty = pos.empty_cells();
    Score best = -INF;
    std::vector<std::pair<std::pair<int, int>, int>> best_moves;

    if (empty == 0) return {index_to_position(-1), -1}; // board full

    const int d = depth.value_or(5) - 1;

    // Scores every piece we could hand over from the current position and keeps
    // the best {cell, piece} pairs. Searched with a full window on purpose: a
    // narrowed beta makes a fail-high indistinguishable from an exact tie, and
    // the tie would then be picked at random against a genuinely better move.
    auto search_gives = [&](Cell cell) {
        Bitboard avail = pos.available();
        while (avail) {
            Piece piece = std::countr_zero(avail);
            pos.give_piece(piece);
            Score score = -negamax(pos, d, -INF, INF);
            pos.undo_give_piece();
            if (score > best) {
                best = score;
                best_moves.clear();
                best_moves.push_back({index_to_position(cell), piece});
            } else if (score == best) {
                best_moves.push_back({index_to_position(cell), piece});
            }
            avail &= (avail - 1);
        }
    };

    if (!piece_in_hand.has_value()) {
        search_gives(-1);   // opening: nothing to place, the move is give-only
    } else {
        while (empty) {
            Cell cell = std::countr_zero(empty);  // C++20
            pos.place_piece(cell);
            // An outright win, or the forced last cell - either way we are done.
            if (move_wins(pos, cell) || pos.empty_cells() == 0) return {index_to_position(cell), -1};
            search_gives(cell);
            pos.undo_place_piece();
            empty &= (empty - 1);  // clear lowest set bit
        }
    }

    if (best_moves.empty()) return {index_to_position(-1), -1};   // no legal move (malformed input)

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, static_cast<int>(best_moves.size()) - 1);
    return best_moves[dist(gen)];   // return one of the best moves
}

Score negamax(Position& pos, int depth, Score alpha, Score beta)
{
    if (depth == 0) return eval(pos);

    Bitboard empty = pos.empty_cells();     // snapshot, before any place
    while (empty) {
        Cell cell = std::countr_zero(empty);  // C++20
        pos.place_piece(cell);
        if (move_wins(pos, cell)) {
            // depth counts down from the root, so a win found with more depth
            // left is a faster win and scores higher.
            pos.undo_place_piece();
            return WIN + depth;
        }
        if (pos.empty_cells() == 0) {   // board full: draw
            pos.undo_place_piece();
            return 0;
        }
        Bitboard avail = pos.available();
        while (avail) {
            Piece piece = std::countr_zero(avail);
            pos.give_piece(piece);
            Score score = -negamax(pos, depth - 1, -beta, -alpha);
            pos.undo_give_piece();
            alpha = std::max(alpha, score);
            if (alpha >= beta) {
                pos.undo_place_piece();
                return alpha;
            }
            avail &= (avail - 1);
        }
        pos.undo_place_piece();
        empty &= (empty - 1);
    }
    return alpha;
}

}  // namespace quarto
