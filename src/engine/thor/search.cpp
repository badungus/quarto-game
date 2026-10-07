#include "search.hpp"
#include "wincheck.hpp"

#include <algorithm>
#include <bit>
#include <chrono>

namespace quarto {

namespace {
// A win scores WIN - n with n the pieces on the board after it (see
// win_score), and a full board holds 16, so every proven score is at least
// this far from 0. Non-proven scores are 0.
constexpr Score PROVEN = WIN - 16;

// Turns a root score into SearchInfo::mate. A win that leaves n pieces on the
// board is placed at ply n - first, counted from 0 at the root, where `first`
// is the piece count after the root's first placement; even plies are the
// root side's. At the opening the root only gives, so the opponent places
// piece 1 at ply 1.
int mate_distance(Score score, const Position &pos) {
  int first = std::popcount(pos.occupied()) + (pos.in_hand() >= 0 ? 1 : 0);
  if (score >= PROVEN)
    return (WIN - score - first) / 2 + 1;
  if (score <= -PROVEN)
    return -((WIN + score - first + 1) / 2);
  return 0;
}
} // namespace

Result Search::search(Position &pos, int depth, InfoFn cb) {
  nodes_ = 0;
  start_time_ = std::chrono::steady_clock::now();

  // negamax_root never returns an empty list, so one pick per iteration is
  // always valid. The same pick is reported and returned.
  Result best{0, {-1, -1}};
  for (int i = 1; i <= depth && !stop_; ++i) {
    std::vector<Result> best_moves = negamax_root(pos, i);
    if (!stop_) {
      // Plain modulo instead of uniform_int_distribution: the distribution's
      // output differs between standard libraries, this does not. The bias is
      // negligible for at most 240 moves.
      best = best_moves[gen_() % best_moves.size()]; // one of the best moves
      // A proven result cannot change with more depth. The score is reported
      // as plain +-WIN; the distance goes into `mate`.
      int mate = mate_distance(best.score, pos);
      if (mate != 0) {
        stop_ = true;
        best.score = mate > 0 ? WIN : -WIN;
      }
      if (cb)
        cb({i, best, nodes_, mate});
    }
  }
  return best;
}

std::vector<Result> Search::negamax_root(Position &pos, int depth) {
  ++nodes_;

  if ((nodes_ & 4095) == 0) {
    if (std::chrono::steady_clock::now() - start_time_ >=
        std::chrono::milliseconds(movetime_)) {
      stop_ = true;
    } // check if movetime has been reached
  }
  Bitboard empty = pos.empty_cells();
  Score best = -INF;

  std::vector<Result> best_moves;

  if (empty == 0)
    return {{0, {-1, -1}}}; // board full

  if (pos.in_hand() < 0 &&
      !stop_) { // starting position, no valid place for hand
    Bitboard avail = pos.available();
    while (avail) {
      Piece piece = std::countr_zero(avail);
      pos.give_piece(piece);
      // Root window (best - 1, INF), see the placement loop below.
      Score score = -negamax(pos, depth - 1, -INF, Score(1 - best));
      pos.undo_give_piece();
      if (score > best) {
        best = score;
        best_moves.clear();
        best_moves.push_back({score, {-1, piece}});
      } else if (score == best) {
        best_moves.push_back({score, {-1, piece}});
      }
      avail &= (avail - 1);
    }
    // Nothing in hand means there is nothing to place: giving is the only move.
    if (best_moves.empty())
      return {{0, {-1, -1}}}; // search stopped
    return best_moves;
  }

  while (empty) {
    Cell cell = std::countr_zero(empty); // C++20
    pos.place_piece(cell);
    if (move_wins(pos, cell)) { // An outright win
      pos.undo_place_piece();
      return {{win_score(pos), {cell, -1}}}; // same scale as negamax
    }

    if (pos.empty_cells() == 0) { // draw
      pos.undo_place_piece();
      return {{0, {cell, -1}}};
    }
    // Score every piece we could hand over after this placement and keep the
    // best {cell, piece} pairs. Root window (best - 1, INF): a worse move
    // fails low at <= best - 1 and is cut off early, while a tie (== best)
    // or a better move (beta is INF) still gets its exact score. Raising
    // alpha to best instead would make a tie indistinguishable from a worse
    // move.
    if (!stop_) {
      Bitboard avail = pos.available();
      while (avail) {
        Piece piece = std::countr_zero(avail);
        pos.give_piece(piece);
        Score score = -negamax(pos, depth - 1, -INF, Score(1 - best));
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
    }
    pos.undo_place_piece();
    empty &= (empty - 1); // clear lowest set bit
  }

  if (best_moves.empty())
    return {{0, {-1, -1}}}; // no legal move or search stopped

  return best_moves;
}

Score Search::negamax(Position &pos, int depth, Score alpha, Score beta) {

  ++nodes_;
  if ((nodes_ & 4095) == 0) {
    if (std::chrono::steady_clock::now() - start_time_ >=
        std::chrono::milliseconds(movetime_)) {
      stop_ = true;
    } // check if movetime has been reached
  }

  // Look for a win on every cell before searching any of them, so a losing
  // give from the parent costs a few win checks instead of whole subtrees.
  // Checked before the horizon too: a win is final at any depth, and eval
  // assumes the piece in hand cannot win.
  if (hand_wins(pos))
    return win_score(pos);

  if (depth == 0 || stop_)
    return eval(pos);

  // No placement below can win anymore, so the loop skips the win check.
  Bitboard empty = pos.empty_cells(); // snapshot, before any place
  while (empty) {
    Cell cell = std::countr_zero(empty); // C++20
    pos.place_piece(cell);
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
