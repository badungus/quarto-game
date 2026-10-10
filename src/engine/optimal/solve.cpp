#include "solve.hpp"
#include "canonical.hpp"
#include "position.hpp"
#include "wincheck.hpp"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstdio>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace quarto {

namespace {
// Some legal move, for positions where every move has the same value.
Move any_move(const Position &pos) {
  Cell cell =
      pos.in_hand() >= 0 ? Cell(std::countr_zero(pos.empty_cells())) : Cell(-1);
  Piece give =
      pos.available() ? Piece(std::countr_zero(pos.available())) : Piece(-1);
  return {cell, give};
}
} // namespace

// The book holds every position up to this many turns in (= pieces placed);
// the jobs are the positions at exactly this ply. 4 because the engine solves
// what lies beyond live, and that must stay under 3 s: on one thread with its
// 2048 MB TT, 100 random positions with 5 pieces placed took at most 1.5 s
// (median 0.34 s), but with 4 pieces placed the median was 4.6 s.
constexpr int SEARCH_START_PLY = 4;

Book read_book(const std::string &path) {
  Book book;
  if (std::FILE *in = std::fopen(path.c_str(), "r")) {
    char line[128];
    while (std::fgets(line, sizeof line, in)) {
      unsigned long long cols;
      unsigned occ;
      int value, cell, give;
      if (std::sscanf(line, "%llx %x %d %d %d", &cols, &occ, &value, &cell,
                      &give) == 5)
        book[{cols, Bitboard(occ)}] = {Score(value),
                                       {Cell(cell), Piece(give)}};
    }
    std::fclose(in);
  }
  return book;
}

SolveResult Solver::full_solve(unsigned threads,
                               const std::string &book_path) {
  const std::vector<Position> jobs = create_positions_();
  std::vector<Canon> canons;
  for (const Position &job : jobs)
    canons.push_back(canonicalize(job));

  // Jobs solved by an earlier, interrupted run.
  Book book = read_book(book_path);
  std::vector<std::size_t> todo;
  for (std::size_t i = 0; i < jobs.size(); ++i)
    if (!book.contains({canons[i].cols, canons[i].occ}))
      todo.push_back(i);
  std::printf("jobs:     %zu, %zu already in %s\n", jobs.size(),
              jobs.size() - todo.size(), book_path.c_str());
  std::fflush(stdout);

  auto write_line = [](std::FILE *f, const CanonKey &key, BookEntry e) {
    std::fprintf(f, "%016llx %04x %d %d %d\n", (unsigned long long)key.first,
                 unsigned(key.second), int(e.value), int(e.move.cell),
                 int(e.move.give));
  };

  std::FILE *out = std::fopen(book_path.c_str(), "a");
  if (!out)
    throw std::runtime_error("cannot write " + book_path);
  std::atomic<std::size_t> next{0};
  std::atomic<std::uint64_t> total_nodes{0};
  std::mutex lock; // guards book, out and stdout
  tt_.new_search();
  {
    std::vector<std::jthread> workers;
    for (unsigned t = 0; t < std::max(threads, 1u); ++t)
      workers.emplace_back([&] {
        // A copy: same TT and settings, its own node count.
        Solver solver(*this);
        for (std::size_t n; (n = next.fetch_add(1)) < todo.size();) {
          const std::size_t i = todo[n];
          Position pos = jobs[i]; // a copy: solve() modifies it
          SolveResult r = solver.solve(pos);
          total_nodes += solver.nodes();
          const CanonKey key = {canons[i].cols, canons[i].occ};
          const BookEntry e = {r.value, to_canonical(r.move, canons[i])};
          std::lock_guard guard(lock);
          book[key] = e;
          // Saved at once, so a crash loses at most the jobs still running.
          write_line(out, key, e);
          std::fflush(out);
          std::printf("job %zu/%zu: %s, %llu nodes\n", n + 1, todo.size(),
                      outcome(r.value), (unsigned long long)solver.nodes());
          std::fflush(stdout);
        }
      });
  } // every thread is joined here, so every job is in the book
  std::fclose(out);
  nodes_ = total_nodes;

  // The jobs start from piece 0 in hand; every first give is equivalent, so
  // the empty board's value is minus that position's, for any give.
  Position pos;
  pos.load(0, {0, 0, 0, 0}, 0);
  std::set<CanonKey> done;
  const Score value = combine(0, pos, book, done);

  // Written to a new file first, then swapped in with one rename that
  // replaces the old book, so at every moment there is a complete book.
  const std::string tmp = book_path + ".tmp";
  std::FILE *f = std::fopen(tmp.c_str(), "w");
  if (!f)
    throw std::runtime_error("cannot write " + tmp);
  std::fprintf(f, "# cols occ value cell give (canonical frame)\n");
  for (const auto &[key, e] : book)
    write_line(f, key, e);
  if (std::fclose(f) != 0)
    throw std::runtime_error("cannot write " + tmp);
  std::filesystem::rename(tmp, book_path); // throws on failure, keeps tmp

  return {Score(-value), {-1, 0}};
}

Score Solver::combine(int ply, Position &pos, Book &book,
                      std::set<CanonKey> &done) {
  const Canon c = canonicalize(pos);
  const CanonKey key = {c.cols, c.occ};
  // A job, or a position this run already combined (as an equivalent one).
  if (ply == SEARCH_START_PLY || done.contains(key))
    return book.at(key).value;
  // Same moves as find_position, which are the moves negamax searches.
  Score best = SOLVED_LOSS;
  Move best_move = EMPTY_MOVE;
  for (Bitboard empty = pos.empty_cells(); empty; empty &= empty - 1) {
    const Cell cell = Cell(std::countr_zero(empty));
    pos.place_piece(cell);
    for (Bitboard safe = pos.available() & ~winning_pieces(pos); safe;
         safe &= safe - 1) {
      const Piece give = Piece(std::countr_zero(safe));
      pos.give_piece(give);
      const Score score = Score(-combine(ply + 1, pos, book, done));
      pos.undo_give_piece();
      if (best_move.cell < 0 || score > best) {
        best = score;
        best_move = {cell, give};
      }
    }
    pos.undo_place_piece();
  }
  // No placement leaves a safe give: every move loses at once, so any one
  // will do.
  if (best_move.cell < 0)
    best_move = {Cell(std::countr_zero(pos.empty_cells())),
                 Piece(std::countr_zero(pos.available()))};
  book[key] = {best, to_canonical(best_move, c)};
  done.insert(key);
  return best;
}

std::vector<Position> Solver::create_positions_() {
  std::vector<Position> positions;
  std::set<CanonKey> seen;
  // Every first give is equivalent, so piece 0 in hand covers them all.
  Position pos;
  pos.load(0, {0, 0, 0, 0}, 0);
  find_position(0, pos, seen, positions);
  return positions;
}

// Adds every position SEARCH_START_PLY turns below `pos` to `positions`,
// one per group of equivalent positions. `seen` holds the canonical form
// of every position visited so far, at any ply: positions at different
// plies have different piece counts, so they never share one.
//
// Only safe gives are followed, as in negamax: a give that lets the
// opponent complete a line loses at once, so it is never the best move
// unless every give does that, and then the position is a proven loss
// that needs no job. So no position here has a piece in hand that can win,
// and no placement needs a win check.
void Solver::find_position(int ply, Position &pos,
                           std::set<CanonKey> &seen,
                           std::vector<Position> &positions) {
  Canon c = canonicalize(pos);
  if (!seen.insert({c.cols, c.occ}).second)
    return; // an equivalent position was already expanded or listed
  if (ply == SEARCH_START_PLY) {
    positions.push_back(pos);
    return;
  }
  for (Bitboard empty = pos.empty_cells(); empty; empty &= empty - 1) {
    pos.place_piece(Cell(std::countr_zero(empty)));
    for (Bitboard safe = pos.available() & ~winning_pieces(pos); safe;
         safe &= safe - 1) {
      pos.give_piece(Piece(std::countr_zero(safe)));
      find_position(ply + 1, pos, seen, positions);
      pos.undo_give_piece();
    }
    pos.undo_place_piece();
  }
}

SolveResult Solver::solve(Position &pos) {
  nodes_ = 0;
  if (pos.empty_cells() == 0)
    return {SOLVED_DRAW, EMPTY_MOVE}; // board full

  Score alpha = SOLVED_LOSS;
  const Score beta = SOLVED_WIN;

  if (pos.in_hand() < 0) {
    // Nothing in hand: giving is the only move. Giving a piece that
    // completes a line loses at once, so only safe pieces are searched.
    // On the empty board every piece is equivalent to every other.
    Bitboard safe = 0;
    if (pos.occupied() == 0)

      safe = Bitboard{1};
    else {
      safe = pos.available() & ~winning_pieces(pos);
    }
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
  ++nodes_;
  // A stopped solve unwinds at once. Its values are meaningless from here on,
  // so nothing below is stored once stopped.
  if (stopped())
    return SOLVED_DRAW;

  const int pieces = std::popcount(pos.occupied());
  // One cell left: the parent gave a piece that cannot complete a line,
  // so placing it fills the board without winning.
  if (pieces == 15)
    return SOLVED_DRAW;

  const bool canon = pieces <= CANON_MAX_PIECES;
  Canon c{};
  TTKey key;
  // The whole position, so TT matches are exact. The parent always gives
  // a piece before calling, so there is always one in hand, and the
  // available pieces are the ones neither placed nor in hand: neither
  // needs any bits.
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
  // A canonical entry is shared by all equivalent positions, so its move
  // is stored in the canonical frame and mapped back on probe.
  auto to_tt = [&](Move m) { return canon ? to_canonical(m, c) : m; };
  // Every entry is searched to the end of the game, so depth never limits
  // a cutoff. It is still stored, as the number of placements left, so
  // the TT keeps the bigger subtrees when slots collide.
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

  // Gives `piece` once the placement on `cell` is made, and searches it.
  // True when it cuts this node off.
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
    if (!stopped())
      tt_.store(key, alpha, depth, to_tt(best_move), Bound::Lower);
    if (best_out)
      *best_out = best_move;
    return alpha;
  };

  // Stage 1: the TT move, before anything else. It may come from an
  // equivalent position or, very rarely, a hash collision, so it is
  // checked first: the cell must be empty, and the piece available and
  // safe after the placement. Searching an unsafe give would break the
  // invariant that the piece in hand cannot win. tt_cell is the TT move's
  // cell, or -1 when there is no TT move to try.
  const Cell tt_cell = entry_move.cell >= 0 && entry_move.give >= 0 &&
                               (pos.empty_cells() >> entry_move.cell & 1)
                           ? entry_move.cell
                           : Cell(-1);
  // The safe gives on tt_cell other than the TT give, for stage 2:
  // placing there was needed anyway to check the TT give, so it is not
  // repeated.
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

  // Every give, after every placement, lets the opponent win with the
  // next piece.
  if (!any_safe) {
    tt_.store(key, SOLVED_LOSS, depth, EMPTY_MOVE, Bound::Exact);
    return SOLVED_LOSS;
  }
  if (!stopped())
    tt_.store(key, alpha, depth, to_tt(best_move),
              alpha > alpha_orig ? Bound::Exact : Bound::Upper);
  if (best_out)
    *best_out = best_move;
  return alpha;
}

} // namespace quarto
