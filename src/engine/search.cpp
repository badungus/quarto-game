#include "search.hpp"
#include "wincheck.hpp"

#include <algorithm>
#include <bit>
#include <random>

namespace quarto {

// Horizon evaluation: the side to move wins outright if any placement of the
// held piece completes a line; otherwise guess a draw. Restores the position.

Move search(Position &pos, int depth) {

  std::vector<Result> best_moves;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0,
                                       static_cast<int>(best_moves.size()) - 1);
  for (int i = 1; i < depth + 1; ++i) {
    best_moves = negamax_root(pos, depth);
    Result random_best_move = best_moves[dist(gen)];
  };

  return best_moves[dist(gen)].move; // return one of the best moves
};

std::vector<Result> negamax_root(Position &pos, int depth) {

  Bitboard empty = pos.empty_cells();
  Score best = -INF;

  std::vector<Result> best_moves;

  if (empty == 0)
    return {{0, {-1, -1}}}; // board full

  // Scores every piece we could hand over from the current position and keeps
  // the best {cell, piece} pairs. Searched with a full window on purpose: a
  // narrowed beta makes a fail-high indistinguishable from an exact tie, and
  // the tie would then be picked at random against a genuinely better move.
  auto search_gives = [&](Cell cell) {
    Bitboard avail = pos.available();
    while (avail) {
      Piece piece = std::countr_zero(avail);
      pos.give_piece(piece);
      Score score = -negamax(pos, depth - 1, -INF, INF);
      pos.undo_give_piece();
      if (score > best) {
        best = score;
        best_moves.clear();
        best_moves.push_back({score, {cell, piece}});
      } else if (score == best) {
        best_moves.push_back({score, {cell, piece}});
      }
      avail &= (avail - 1);
    }
  };

  if (pos.in_hand() < 0) {
    search_gives(-1); // opening: nothing to place, the move is give-only
  } else {
    while (empty) {
      Cell cell = std::countr_zero(empty); // C++20
      pos.place_piece(cell);
      // An outright win, or the forced last cell - either way we are done.
      if (move_wins(pos, cell))
        return {{INF, {cell, -1}}};

      if (pos.empty_cells() == 0) {
        return {{0, {cell, -1}}};
      }
      search_gives(cell);
      pos.undo_place_piece();
      empty &= (empty - 1); // clear lowest set bit
    }
  }

  if (best_moves.empty())
    return {{0, {-1, -1}}}; // no legal move (malformed input)

  return best_moves;
}

Score negamax(Position &pos, int depth, Score alpha, Score beta) {
  if (depth == 0)
    return eval(pos);

  Bitboard empty = pos.empty_cells(); // snapshot, before any place
  while (empty) {
    Cell cell = std::countr_zero(empty); // C++20
    pos.place_piece(cell);
    if (move_wins(pos, cell)) {
      // depth counts down from the root, so a win found with more depth
      // left is a faster win and scores higher.
      pos.undo_place_piece();
      return WIN + depth;
    }
    if (pos.empty_cells() == 0) { // board full: draw
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

} // namespace quarto
