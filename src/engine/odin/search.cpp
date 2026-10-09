#include "search.hpp"
#include "canonical.hpp"
#include "types.hpp"
#include "wincheck.hpp"
#include <bit>
#include <chrono>

namespace quarto {

namespace {
// A win scores WIN - n with n the pieces on the board after it (see
// win_score), and a full board holds 16, so every proven score is at least
// this far from 0. Non-proven scores are 0.
constexpr Score PROVEN = WIN - 16;

// Positions with at most this many pieces on the board are keyed by their
// canonical form, so equivalent positions share a TT entry; deeper ones by
// the free incremental Zobrist key, since canonicalize costs a few hundred
// operations and transpositions through symmetry get rarer and cheaper with
// depth. The piece count decides, so a position always gets the same kind of
// key. Tune by benchmark.
constexpr int CANON_MAX_PIECES = 10;

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

int pop_first(Bitboard &set, int first) {
  int i = (first >= 0 && (set >> first & 1)) ? first : std::countr_zero(set);
  set &= ~(Bitboard{1} << i);
  return i;
}
} // namespace

Result Search::search(Position &pos, int depth, InfoFn cb) {
  nodes_ = 0;
  start_time_ = std::chrono::steady_clock::now();
  tt_.new_search();

  Result best{0, {-1, -1}};
  for (int i = 1; i <= depth && !stop_; ++i) {
    std::vector<Result> best_moves = negamax_root(pos, i, best.move);
    if (!stop_) {
      best = best_moves[gen_() % best_moves.size()]; // one of the best moves
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

std::vector<Result> Search::negamax_root(Position &pos, int depth, Move first) {
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
  bool any_safe = false; // some placement left a give that does not lose

  if (empty == 0)
    return {{0, {-1, -1}}}; // board full

  if (pos.in_hand() < 0 &&
      !stop_) { // starting position, no valid place for hand
    Bitboard avail = pos.available();
    while (avail) {
      Piece piece = pop_first(avail, first.give);
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
    }
    // Nothing in hand means there is nothing to place: giving is the only move.
    if (best_moves.empty())
      return {{0, {-1, -1}}}; // search stopped
    return best_moves;
  }

  while (empty) {
    Cell cell = pop_first(empty, first.cell);
    pos.place_piece(cell);
    if (move_wins(pos, cell)) { // An outright win
      pos.undo_place_piece();
      return {{win_score(pos), {cell, -1}}}; // same scale as negamax
    }

    if (pos.empty_cells() == 0) { // draw
      pos.undo_place_piece();
      return {{0, {cell, -1}}};
    }

    if (!stop_) {
      Bitboard safe_gives = ~winning_pieces(pos);
      Bitboard avail = safe_gives & pos.available();
      while (avail) {
        Piece piece = pop_first(avail, cell == first.cell ? first.give : -1);
        any_safe = true;
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
      }
    }
    pos.undo_place_piece();
  }

  // No placement leaves a safe give: every move loses at once, so all of them
  // tie at that loss (negamax only searches safe gives, so none were scored).
  if (!any_safe && !stop_) {
    Score loss = Score(-(win_score(pos) - 1));
    for (Bitboard e = pos.empty_cells(); e; e &= (e - 1))
      for (Bitboard a = pos.available(); a; a &= (a - 1))
        best_moves.push_back(
            {loss, {Cell(std::countr_zero(e)), Piece(std::countr_zero(a))}});
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

  const bool canon = std::popcount(pos.occupied()) <= CANON_MAX_PIECES;
  Canon c{};
  Key key;
  if (canon) {
    c = canonicalize(pos);
    key = c.key();
  } else {
    key = pos.hash();
  }
  // A canonical entry is shared by all equivalent positions, so its move is
  // stored in the canonical frame and mapped back on probe. Storing real
  // moves there would order the wrong move first in every other equivalent
  // position: still correct, but much slower.
  auto to_tt = [&](Move m) { return canon ? to_canonical(m, c) : m; };

  const TTEntry *entry = tt_.probe(key);
  Move entry_move = EMPTY_MOVE;
  if (entry)
    entry_move = canon ? from_canonical(entry->move, c) : entry->move;
  if (entry) {
    // A proven win as a lower bound or a proven loss as an upper bound is the
    // true game result, so it holds at any depth; the opposite bounds do not.
    bool proven = (entry->bound != Bound::Upper && entry->score >= PROVEN) ||
                  (entry->bound != Bound::Lower && entry->score <= -PROVEN);
    if (entry->depth >= depth || proven) {
      if (entry->bound == Bound::Exact)
        return entry->score;
      if (entry->bound == Bound::Lower && entry->score >= beta)
        return entry->score;
      if (entry->bound == Bound::Upper && entry->score <= alpha)
        return entry->score;
    }
  }

  if (std::popcount(pos.occupied()) == 15)
    return 0;

  if (depth == 0 || stop_)
    return eval(pos);

  Score alpha_orig = alpha;
  // No placement below can win anymore, so the loops skip the win check.
  Move best_move = EMPTY_MOVE;
  bool any_safe = false; // some placement left a give that does not lose

  // Gives `piece` once the placement on `cell` is made, and searches it. True
  // when it cuts this node off.
  auto search_give = [&](Cell cell, Piece piece) {
    pos.give_piece(piece);
    Score score = -negamax(pos, depth - 1, -beta, -alpha);
    pos.undo_give_piece();
    if (score > alpha) {
      alpha = score;
      best_move = {cell, piece};
    }
    return alpha >= beta;
  };
  auto cutoff = [&] {
    if (!stop_)
      tt_.store(key, alpha, depth, to_tt(best_move), Bound::Lower);
    history_[best_move.cell][best_move.give]++;
    give_history_[best_move.give]++;
    return alpha;
  };

  // Stage 1: the TT move, before anything else. It may come from an
  // equivalent position or, very rarely, a hash collision, so it is checked
  // first: the cell must be empty, and the piece available and safe after the
  // placement. Searching an unsafe give would break the invariant that the
  // piece in hand cannot win.
  // tt_cell is the TT move's cell, or -1 when there is no TT move to try.
  const Cell tt_cell = entry_move.cell >= 0 && entry_move.give >= 0 &&
                               (pos.empty_cells() >> entry_move.cell & 1)
                           ? entry_move.cell
                           : Cell(-1);
  // The safe gives on tt_cell other than the TT give, for stage 2: placing
  // there was needed anyway to check the TT give, so it is not repeated.
  Bitboard tt_cell_safe = 0;
  if (tt_cell >= 0) {
    pos.place_piece(tt_cell);
    Bitboard safe = pos.available() & ~winning_pieces(pos);
    if (safe >> entry_move.give & 1) {
      any_safe = true;
      if (search_give(tt_cell, entry_move.give)) {
        pos.undo_place_piece();
        return cutoff();
      }
    }
    tt_cell_safe = safe & ~(Bitboard{1} << entry_move.give);
    pos.undo_place_piece();
  }

  // Stage 2: every other move, cells and pieces in index order.
  for (Bitboard empty = pos.empty_cells(); empty; empty &= empty - 1) {
    Cell cell = Cell(std::countr_zero(empty));
    pos.place_piece(cell);
    Bitboard avail = cell == tt_cell
                         ? tt_cell_safe
                         : Bitboard(pos.available() & ~winning_pieces(pos));
    if (avail)
      any_safe = true;
    for (; avail; avail &= avail - 1) {
      if (search_give(cell, Piece(std::countr_zero(avail)))) {
        pos.undo_place_piece();
        return cutoff();
      }
    }
    pos.undo_place_piece();
  }
  // Every give, after every placement, lets the opponent win with the next
  // piece: an exact loss, independent of depth.
  if (!any_safe) {
    Score loss = Score(-(win_score(pos) - 1));
    tt_.store(key, loss, depth, EMPTY_MOVE, Bound::Exact);
    return loss;
  }
  if (!stop_)
    tt_.store(key, alpha, depth, to_tt(best_move),
              alpha > alpha_orig ? Bound::Exact : Bound::Upper);
  return alpha;
}

} // namespace quarto
