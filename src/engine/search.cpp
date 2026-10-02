#include "search.hpp"
#include "wincheck.hpp"

#include <algorithm>
#include <bit>
#include <chrono>
#include <random>

namespace quarto {

Result Search::search(Position &pos, int depth, InfoFn cb) {
  nodes_ = 0;
  start_time_ = std::chrono::steady_clock::now();
  std::random_device rd;
  std::mt19937 gen(rd());

  // negamax_root never returns an empty list, so one pick per iteration is
  // always valid. The same pick is reported and returned.
  Result best{0, {-1, -1}};
  for (int i = 1; i <= depth && !stop_; ++i) {
    std::vector<Result> best_moves = negamax_root(pos, i);
    if (!stop_) {
      std::uniform_int_distribution<> dist(
          0, static_cast<int>(best_moves.size()) - 1);
      best = best_moves[dist(gen)]; // one of the best moves
      if (best.score >= WIN - i) {
        stop_ = true;
        best.score = WIN;
      }
      if (best.score <= -WIN + i) {
        stop_ = true;
        best.score = -WIN;
      }
      if (cb)
        cb({i, best});
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
      Score score = -negamax(pos, depth - 1, -INF, INF);
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
      return {{WIN, {cell, -1}}};
    }

    if (pos.empty_cells() == 0) { // draw
      pos.undo_place_piece();
      return {{0, {cell, -1}}};
    }
    // Score every piece we could hand over after this placement and keep the
    // best {cell, piece} pairs. Searched with a full window on purpose: a
    // narrowed beta makes a fail-high indistinguishable from an exact tie, and
    // the tie would then be picked at random against a genuinely better move.
    if (!stop_) {
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

  if (depth == 0 || stop_)
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
