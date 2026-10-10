#include "solve.hpp"
#include "canonical.hpp"
#include "wincheck.hpp"

#include <bit>

namespace quarto {

namespace {
// Some legal move, for positions where every move has the same value.
Move any_move(const Position &pos) {
  Cell cell = pos.in_hand() >= 0 ? Cell(std::countr_zero(pos.empty_cells()))
                                 : Cell(-1);
  Piece give = pos.available() ? Piece(std::countr_zero(pos.available()))
                               : Piece(-1);
  return {cell, give};
}
} // namespace

SolveResult Solver::solve(Position &pos) {
  nodes_ = 0;
  tt_.new_search();

  if (pos.empty_cells() == 0)
    return {SOLVED_DRAW, EMPTY_MOVE}; // board full

  Score alpha = SOLVED_LOSS;
  const Score beta = SOLVED_WIN;

  if (pos.in_hand() < 0) {
    // Nothing in hand: giving is the only move. Giving a piece that completes
    // a line loses at once, so only safe pieces are searched.
    Bitboard safe = pos.available() & ~winning_pieces(pos);
    // On the empty board every piece is equivalent to every other.
    if (pos.occupied() == 0)
      safe &= -safe;
    Move best = EMPTY_MOVE;
    for (; safe && alpha < beta; safe &= safe - 1) {
      Piece piece = Piece(std::countr_zero(safe));
      pos.give_piece(piece);
      Score score = -negamax(pos, -beta, -alpha);
      pos.undo_give_piece();
      if (score > alpha) {
        alpha = score;
        best = {-1, piece};
      }
    }
    return {alpha, best.give >= 0 ? best : any_move(pos)};
  }

  // The piece in hand may win outright, or fill the board.
  for (Bitboard e = pos.empty_cells(); e; e &= e - 1) {
    Cell cell = Cell(std::countr_zero(e));
    pos.place_piece(cell);
    bool win = move_wins(pos, cell);
    pos.undo_place_piece();
    if (win)
      return {SOLVED_WIN, {cell, -1}};
  }
  if (std::popcount(pos.occupied()) == 15)
    return {SOLVED_DRAW, {Cell(std::countr_zero(pos.empty_cells())), -1}};

  Move best = EMPTY_MOVE;
  Score value = negamax(pos, alpha, beta, &best);
  // No move beat a loss: all of them lose, so any one will do.
  return {value, best.cell >= 0 ? best : any_move(pos)};
}

Score Solver::negamax(Position &pos, Score alpha, Score beta, Move *best_out) {
  if ((++nodes_ & (PROGRESS_INTERVAL - 1)) == 0 && on_progress_)
    on_progress_(nodes_);

  const int pieces = std::popcount(pos.occupied());
  // One cell left: the parent gave a piece that cannot complete a line, so
  // placing it fills the board without winning.
  if (pieces == 15)
    return SOLVED_DRAW;

  const bool canon = pieces <= canon_max_pieces_;
  Canon c{};
  TTKey key;
  // The whole position, so TT matches are exact. The parent always gives a
  // piece before calling, so there is always one in hand, and the available
  // pieces are the ones neither placed nor in hand: neither needs any bits.
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
  // A canonical entry is shared by all equivalent positions, so its move is
  // stored in the canonical frame and mapped back on probe.
  auto to_tt = [&](Move m) { return canon ? to_canonical(m, c) : m; };
  // Every entry is searched to the end of the game, so depth never limits a
  // cutoff. It is still stored, as the number of placements left, so the TT
  // keeps the bigger subtrees when slots collide.
  const auto depth = std::uint8_t(16 - pieces);

  const std::optional<TTHit> entry = tt_.probe(key);
  Move entry_move = EMPTY_MOVE;
  if (entry) {
    entry_move = canon ? from_canonical(entry->move, c) : entry->move;
    // The root needs a move, not just a value, so it always searches.
    if (!best_out) {
      if (entry->bound == Bound::Exact)
        return entry->score;
      if (entry->bound == Bound::Lower && entry->score >= beta)
        return entry->score;
      if (entry->bound == Bound::Upper && entry->score <= alpha)
        return entry->score;
    }
  }

  const Score alpha_orig = alpha;
  Move best_move = EMPTY_MOVE;
  bool any_safe = false; // some placement left a give that does not lose

  // Gives `piece` once the placement on `cell` is made, and searches it. True
  // when it cuts this node off.
  auto search_give = [&](Cell cell, Piece piece) {
    pos.give_piece(piece);
    Score score = -negamax(pos, -beta, -alpha);
    pos.undo_give_piece();
    if (score > alpha) {
      alpha = score;
      best_move = {cell, piece};
    }
    return alpha >= beta;
  };
  auto cutoff = [&] {
    tt_.store(key, alpha, depth, to_tt(best_move), Bound::Lower);
    if (best_out)
      *best_out = best_move;
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
  // piece.
  if (!any_safe) {
    tt_.store(key, SOLVED_LOSS, depth, EMPTY_MOVE, Bound::Exact);
    return SOLVED_LOSS;
  }
  tt_.store(key, alpha, depth, to_tt(best_move),
            alpha > alpha_orig ? Bound::Exact : Bound::Upper);
  if (best_out)
    *best_out = best_move;
  return alpha;
}

} // namespace quarto
