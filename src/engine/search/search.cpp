#include "search.hpp"
#include "canonical.hpp"
#include "features.hpp"
#include "ordering.hpp"
#include "wincheck.hpp"

#include <algorithm>
#include <bit>
#include <chrono>

namespace quarto {

namespace {
// A win scores WIN - n with n the pieces on the board after it (see
// win_score), and a full board holds 16, so every proven score is at least
// this far from 0. Non-proven scores are 0. Used with PIECE_COUNT_SCORE.
constexpr Score PROVEN = WIN - 16;

// CANONICAL: positions with at most this many pieces on the board are keyed by
// their canonical form, so equivalent positions share a TT entry; deeper ones
// by the position as it is, since canonicalize costs a few hundred operations
// and transpositions through symmetry get rarer and cheaper with depth. The
// piece count decides, so a position always gets the same kind of key. Tune by
// benchmark.
constexpr int CANON_MAX_PIECES = 10;

// Turns a root score from an iteration of `depth` into SearchInfo::mate.
//
// PIECE_COUNT_SCORE: a win that leaves n pieces on the board is placed at ply
// n - first, counted from 0 at the root, where `first` is the piece count after
// the root's first placement. At the opening the root only gives, so the
// opponent places piece 1 at ply 1.
//
// Otherwise: a win found with `d` depth left scores WIN + d and happens at ply
// depth - d.
//
// Even plies are the root side's.
int mate_distance(Score score, int depth, const Position &pos) {
  if constexpr (PIECE_COUNT_SCORE) {
    int first = std::popcount(pos.occupied()) + (pos.in_hand() >= 0 ? 1 : 0);
    if (score >= PROVEN)
      return (WIN - score - first) / 2 + 1;
    if (score <= -PROVEN)
      return -((WIN + score - first + 1) / 2);
  } else {
    if (score >= WIN)
      return (depth - (score - WIN)) / 2 + 1;
    if (score <= -WIN)
      return -((depth - (-score - WIN) + 1) / 2);
  }
  return 0;
}

// Beta for the search of a root move's reply, given the root's best score so
// far: the root window is (best - 1, INF) with NARROW_ROOT_WINDOW, else
// (-INF, INF), and the reply sees it negated. Raising the root's alpha to best
// instead would make a tie indistinguishable from a worse move, and ties are
// all kept for the random pick.
Score reply_beta(Score best) {
  if constexpr (NARROW_ROOT_WINDOW)
    return Score(1 - best);
  return INF;
}

// Removes and returns `first` from `set` if it is in it, otherwise the lowest
// index.
int pop_first(Bitboard &set, int first) {
  int i = (first >= 0 && (set >> first & 1)) ? first : std::countr_zero(set);
  set &= ~(Bitboard{1} << i);
  return i;
}

// The pieces searched as gives after a placement: the safe ones with
// SAFE_GIVES, else every available one.
Bitboard gives(Position &pos) {
  if constexpr (SAFE_GIVES)
    return pos.available() & ~winning_pieces(pos);
  return pos.available();
}

// The score of a lost position: every placement + give lets the opponent win
// with the next placement, one more piece than a win from here would place.
Score loss(const Position &pos) { return Score(-(win_score(pos) - 1)); }

// The score of a win by the piece in hand, placed from `pos` with `depth`
// plies left.
Score win(const Position &pos, int depth) {
  if constexpr (PIECE_COUNT_SCORE)
    return win_score(pos);
  // depth counts down from the root, so a win found with more depth left is
  // a faster win and scores higher.
  return Score(WIN + depth);
}
} // namespace

Result Search::search(Position &pos, int depth, InfoFn cb) {
  nodes_ = 0;
  start_time_ = std::chrono::steady_clock::now();
  if constexpr (TT)
    tt_.new_search();

  // negamax_root never returns an empty list, so one pick per iteration is
  // always valid. The same pick is reported and returned.
  Result best{0, {-1, -1}};
  for (int i = 1; i <= depth && !stop_; ++i) {
    // ROOT_BEST_FIRST: the previous iteration's pick first.
    std::vector<Result> best_moves =
        negamax_root(pos, i, ROOT_BEST_FIRST ? best.move : EMPTY_MOVE);
    if (!stop_) {
      // Plain modulo instead of uniform_int_distribution: the distribution's
      // output differs between standard libraries, this does not. The bias is
      // negligible for at most 240 moves.
      best = best_moves[gen_() % best_moves.size()]; // one of the best moves
      // A proven result cannot change with more depth. The score is reported
      // as plain +-WIN; the distance goes into `mate`.
      int mate = mate_distance(best.score, i, pos);
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

std::vector<Result> Search::negamax_root(Position &pos, int depth,
                                         Move first) {
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
  bool any_safe = false; // some placement left a give to search

  if (empty == 0)
    return {{0, {-1, -1}}}; // board full

  if (pos.in_hand() < 0 &&
      !stop_) { // starting position, no valid place for hand
    Bitboard avail = pos.available();
    while (avail) {
      Piece piece = pop_first(avail, first.give);
      pos.give_piece(piece);
      Score score = -negamax(pos, depth - 1, -INF, reply_beta(best));
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
      return {{win(pos, depth), {cell, -1}}}; // same scale as negamax
    }

    if (pos.empty_cells() == 0) { // draw
      pos.undo_place_piece();
      return {{0, {cell, -1}}};
    }
    // Score every piece we could hand over after this placement and keep the
    // best {cell, piece} pairs. The window keeps every tie exact (see
    // reply_beta).
    if (!stop_) {
      Bitboard avail = gives(pos);
      while (avail) {
        Piece piece = pop_first(avail, cell == first.cell ? first.give : -1);
        any_safe = true;
        pos.give_piece(piece);
        Score score = -negamax(pos, depth - 1, -INF, reply_beta(best));
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

  // SAFE_GIVES: no placement leaves a safe give, so every move loses at once
  // and all of them tie at that loss (none were searched).
  if constexpr (SAFE_GIVES)
    if (!any_safe && !stop_)
      for (Bitboard e = pos.empty_cells(); e; e &= (e - 1))
        for (Bitboard a = pos.available(); a; a &= (a - 1))
          best_moves.push_back({loss(pos),
                                {Cell(std::countr_zero(e)),
                                 Piece(std::countr_zero(a))}});

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

  // Checked before the horizon too: a win is final at any depth. With
  // SAFE_GIVES the parent only gives safe pieces, so it cannot happen.
  if constexpr (WIN_CHECK_AT_ENTRY && !SAFE_GIVES)
    if (hand_wins(pos))
      return win(pos, depth);

  TTKey key{};
  Move entry_move = EMPTY_MOVE; // the TT's best move for this position
  // CANONICAL: whether this position is keyed by its canonical form, and the
  // transformation to it.
  bool canon = false;
  Canon c{};
  // A canonical entry is shared by all equivalent positions, so its move is
  // stored in the canonical frame and mapped back on probe. Storing real
  // moves there would order the wrong move first in every other equivalent
  // position: still correct, but much slower.
  auto to_tt = [&](Move m) { return canon ? to_canonical(m, c) : m; };
  if constexpr (TT) {
    // The whole position, so TT matches are exact. The parent always gives a
    // piece before calling, so there is always one in hand, and the available
    // pieces are the ones neither placed nor in hand: neither needs any bits.
    if constexpr (CANONICAL)
      canon = std::popcount(pos.occupied()) <= CANON_MAX_PIECES;
    if (canon) {
      // In canonical form the hand is piece 0.
      c = canonicalize(pos);
      key = {c.cols, c.occ};
    } else {
      const auto &p = pos.props();
      key = {std::uint64_t(p[0]) | std::uint64_t(p[1]) << 16 |
                 std::uint64_t(p[2]) << 32 | std::uint64_t(p[3]) << 48,
             std::uint32_t(pos.occupied() | pos.in_hand() << 16)};
    }

    const std::optional<TTHit> entry = tt_.probe(key);
    if (entry)
      entry_move = canon ? from_canonical(entry->move, c) : entry->move;
    if (entry) {
      // A proven win as a lower bound or a proven loss as an upper bound is
      // the true game result, so it holds at any depth; the opposite bounds
      // do not.
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
  }

  // The parent only gives safe pieces, so the piece in hand cannot win, and
  // with 15 pieces on the board placing it fills the board: a draw.
  if constexpr (SAFE_GIVES)
    if (std::popcount(pos.occupied()) == 15)
      return 0;

  if (depth == 0 || stop_)
    return eval(pos);

  Score alpha_orig = alpha;
  Move best_move = EMPTY_MOVE; // the move that raised alpha, for the TT
  bool any_safe = false; // some placement left a give to search

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
  // The return value of a node that was cut off.
  auto cutoff = [&] {
    if constexpr (TT)
      if (!stop_)
        tt_.store(key, alpha, depth, to_tt(best_move), Bound::Lower);
    return alpha;
  };

  // TT_MOVE: the TT move, before anything else. A hash collision can hand
  // over a move from another position, so it is checked first: the cell must
  // be empty and the piece available. The give is searched only if it is also
  // safe after the placement: an unsafe give would break the invariant that
  // the piece in hand cannot win.
  bool tt_move_legal = false;
  if constexpr (TT_MOVE) {
    tt_move_legal =
        entry_move.cell >= 0 && pos.empty_cells() >> entry_move.cell & 1 &&
        entry_move.give >= 0 && pos.available() >> entry_move.give & 1;
    if (tt_move_legal) {
      pos.place_piece(entry_move.cell);
      if (gives(pos) >> entry_move.give & 1) {
        any_safe = true;
        if (search_give(entry_move.cell, entry_move.give)) {
          pos.undo_place_piece();
          return cutoff();
        }
      }
      pos.undo_place_piece();
    }
  }

  Bitboard empty = pos.empty_cells(); // snapshot, before any place
  // THREAT_ORDERING: the cells by threat prior, else in index order.
  std::array<Score, 16> prior;
  if constexpr (THREAT_ORDERING)
    prior = compute_prior(pos);
  while (empty) {
    Cell cell;
    if constexpr (THREAT_ORDERING)
      cell = best_cell(empty, prior);
    else
      cell = Cell(pop_first(empty, -1));
    pos.place_piece(cell);
    // With WIN_CHECK_AT_ENTRY or SAFE_GIVES no placement here can win.
    if constexpr (!WIN_CHECK_AT_ENTRY)
      if (move_wins(pos, cell)) {
        pos.undo_place_piece();
        return win(pos, depth);
      }
    if constexpr (!SAFE_GIVES)
      if (pos.empty_cells() == 0) { // board full: draw
        pos.undo_place_piece();
        return 0;
      }

    Bitboard avail = gives(pos);
    // The TT move was searched already.
    if (tt_move_legal && entry_move.cell == cell)
      avail &= ~(Bitboard{1} << entry_move.give);
    if (avail)
      any_safe = true;
    while (avail) {
      Piece piece = std::countr_zero(avail);
      avail &= (avail - 1);
      if (search_give(cell, piece)) {
        pos.undo_place_piece();
        return cutoff();
      }
    }
    pos.undo_place_piece();
  }
  // SAFE_GIVES: every give, after every placement, lets the opponent win with
  // the next piece. An exact loss, independent of depth.
  if constexpr (SAFE_GIVES)
    if (!any_safe) {
      if constexpr (TT)
        tt_.store(key, loss(pos), depth, EMPTY_MOVE, Bound::Exact);
      return loss(pos);
    }
  if constexpr (TT)
    if (!stop_)
      tt_.store(key, alpha, depth, to_tt(best_move),
                alpha > alpha_orig ? Bound::Exact : Bound::Upper);
  return alpha;
}

} // namespace quarto
