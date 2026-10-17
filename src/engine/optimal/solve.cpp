#include "solve.hpp"
#include "canonical.hpp"
#include "position.hpp"
#include "wincheck.hpp"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <map>
#include <mutex>
#include <numeric>
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

// Removes and returns `first` from `set` if it is in it, otherwise the lowest
// index.
int pop_first(Bitboard &set, int first) {
  int i = (first >= 0 && (set >> first & 1)) ? first : std::countr_zero(set);
  set &= ~(Bitboard{1} << i);
  return i;
}

// The empty cell with the lowest prior, lowest index on ties.
int lowest_prior(Bitboard set, const std::array<int, 16> &prior) {
  int best = std::countr_zero(set);
  for (Bitboard b = Bitboard(set & (set - 1)); b; b &= b - 1) {
    const int i = std::countr_zero(b);
    if (prior[i] < prior[best])
      best = i;
  }
  return best;
}
} // namespace

// The jobs are the positions this many turns in (= pieces placed), and the
// book holds every position above them. 6 because the engine solves what lies
// beyond the book live, and that must be quick: in a sample of 400, positions
// with 6 pieces placed took a median 0.04 s (max 0.2 s), against 0.45 s with
// 5 and 4.8 s with 4. That makes 26.0 million jobs and a book of 1.86 million
// positions.
constexpr int SEARCH_START_PLY = 4;

Position canonical_position(const CanonKey &key) {
  Position pos;
  pos.load(key.second,
           {Bitboard(key.first), Bitboard(key.first >> 16),
            Bitboard(key.first >> 32), Bitboard(key.first >> 48)},
           0);
  return pos;
}

Book read_book(const std::string &path) {
  Book book;
  std::FILE *in = std::fopen(path.c_str(), "rb");
  if (!in)
    return book;
  char magic[sizeof BOOK_MAGIC];
  if (std::fread(magic, 1, sizeof magic, in) != sizeof magic ||
      !std::equal(magic, magic + sizeof magic, BOOK_MAGIC)) {
    // The older text book.
    std::rewind(in);
    char line[128];
    while (std::fgets(line, sizeof line, in)) {
      unsigned long long cols;
      unsigned occ;
      int value, cell, give;
      if (std::sscanf(line, "%llx %x %d %d %d", &cols, &occ, &value, &cell,
                      &give) == 5 &&
          cell >= 0 && cell < 16 && give >= 0 && give < 16) {
        BookEntry e{Score(value), {}};
        e.gives[cell] = Bitboard(1u << give);
        book[{cols, Bitboard(occ)}] = e;
      }
    }
  } else {
    std::uint64_t cols;
    Bitboard occ, cells;
    Score value;
    while (std::fread(&cols, sizeof cols, 1, in) == 1 &&
           std::fread(&occ, sizeof occ, 1, in) == 1 &&
           std::fread(&value, sizeof value, 1, in) == 1 &&
           std::fread(&cells, sizeof cells, 1, in) == 1) {
      BookEntry e{value, {}};
      for (; cells; cells &= cells - 1)
        if (std::fread(&e.gives[std::countr_zero(cells)], sizeof(Bitboard), 1,
                       in) != 1)
          break; // a cut-off last record: dropped
      if (!cells)
        book[{cols, occ}] = e;
    }
  }
  std::fclose(in);
  return book;
}

void write_book(const std::string &path, const Book &book) {
  const std::string tmp = path + ".tmp";
  std::FILE *f = std::fopen(tmp.c_str(), "wb");
  if (!f)
    throw std::runtime_error("cannot write " + tmp);
  std::fwrite(BOOK_MAGIC, 1, sizeof BOOK_MAGIC, f);
  for (const auto &[key, e] : book) {
    Bitboard cells = 0;
    for (int c = 0; c < 16; ++c)
      if (e.gives[c])
        cells |= Bitboard(1u << c);
    std::fwrite(&key.first, sizeof key.first, 1, f);
    std::fwrite(&key.second, sizeof key.second, 1, f);
    std::fwrite(&e.value, sizeof e.value, 1, f);
    std::fwrite(&cells, sizeof cells, 1, f);
    for (int c = 0; c < 16; ++c)
      if (e.gives[c])
        std::fwrite(&e.gives[c], sizeof e.gives[c], 1, f);
  }
  if (std::fclose(f) != 0)
    throw std::runtime_error("cannot write " + tmp);
  std::filesystem::rename(tmp, path); // throws on failure, keeps tmp
}

SolveResult Solver::full_solve(unsigned threads, const std::string &book_path) {
  // Solved in the order they were found, each from the position as it was
  // reached: neighbouring jobs then share most of their board, in the same
  // frame, so their subtrees meet in the TT. On ply-4 jobs that is 1.3x
  // faster than solving them by sorted key, from canonical positions.
  const std::vector<Job> jobs = create_jobs_();
  // Job indices sorted by key, to find a job by binary search.
  std::vector<std::uint32_t> by_key(jobs.size());
  std::iota(by_key.begin(), by_key.end(), 0u);
  std::sort(by_key.begin(), by_key.end(), [&](std::uint32_t a, std::uint32_t b) {
    return jobs[a].key < jobs[b].key;
  });
  // The index of the job with `key`, or jobs.size() if there is none.
  auto job_index = [&](const CanonKey &key) {
    auto it = std::lower_bound(
        by_key.begin(), by_key.end(), key,
        [&](std::uint32_t i, const CanonKey &k) { return jobs[i].key < k; });
    return it != by_key.end() && jobs[*it].key == key ? std::size_t(*it)
                                                      : jobs.size();
  };

  // Values of jobs solved by an earlier, interrupted run.
  constexpr Score UNSOLVED = std::numeric_limits<Score>::min();
  std::vector<Score> values(jobs.size(), UNSOLVED);
  const std::string checkpoint = book_path + ".jobs";
  if (std::FILE *in = std::fopen(checkpoint.c_str(), "r")) {
    unsigned long long cols;
    unsigned occ;
    int value;
    while (std::fscanf(in, "%llx %x %d", &cols, &occ, &value) == 3) {
      const CanonKey key = {cols, Bitboard(occ)};
      const std::size_t i = job_index(key);
      // A win or loss from a run without distances has none to reuse.
      const bool lacks_distance = value != 0 && std::abs(value) < PROVEN;
      if (i < jobs.size() && !(distances_ && lacks_distance))
        values[i] = Score(value);
    }
    std::fclose(in);
  }
  std::vector<std::size_t> todo;
  for (std::size_t i = 0; i < jobs.size(); ++i)
    if (values[i] == UNSOLVED)
      todo.push_back(i);
  std::printf("jobs:     %zu, %zu already in %s\n", jobs.size(),
              jobs.size() - todo.size(), checkpoint.c_str());
  std::fflush(stdout);

  std::FILE *out = std::fopen(checkpoint.c_str(), "a");
  if (!out)
    throw std::runtime_error("cannot write " + checkpoint);
  std::atomic<std::size_t> next{0};
  std::atomic<std::uint64_t> total_nodes{0};
  std::mutex lock; // guards out and stdout
  tt_.new_search();
  {
    std::vector<std::jthread> workers;
    for (unsigned t = 0; t < std::max(threads, 1u); ++t)
      workers.emplace_back([&] {
        // A copy: same TT and settings, its own node count.
        Solver solver(*this);
        for (std::size_t n; (n = next.fetch_add(1)) < todo.size();) {
          const Job &job = jobs[todo[n]];
          Position pos;
          pos.load(job.occ,
                   {Bitboard(job.props), Bitboard(job.props >> 16),
                    Bitboard(job.props >> 32), Bitboard(job.props >> 48)},
                   job.hand);
          const Score value = solver.solve(pos).value;
          total_nodes += solver.nodes();
          values[todo[n]] = value; // its own slot: no lock needed
          std::lock_guard guard(lock);
          // Saved at once, so a crash loses at most the jobs still running.
          std::fprintf(out, "%016llx %04x %d\n",
                       (unsigned long long)job.key.first,
                       unsigned(job.key.second), int(value));
          std::fflush(out);
          // Millions of jobs: a line per thousand is enough.
          if ((n + 1) % 1000 == 0 || n + 1 == todo.size()) {
            std::printf("job %zu/%zu\n", n + 1, todo.size());
            std::fflush(stdout);
          }
        }
      });
  } // every thread is joined here, so every job has its value
  std::fclose(out);
  nodes_ = total_nodes;

  // The jobs start from piece 0 in hand; every first give is equivalent, so
  // the empty board's value is minus that position's, for any give.
  Position pos;
  pos.load(0, {0, 0, 0, 0}, 0);
  std::vector<std::pair<CanonKey, Score>> solved;
  solved.reserve(jobs.size());
  for (std::uint32_t i : by_key)
    solved.push_back({jobs[i].key, values[i]});
  Book book;
  const Score value = combine(0, pos, solved, book);
  write_book(book_path, book);
  std::filesystem::remove(checkpoint);

  return {Score(-value), {-1, 0}};
}

Score Solver::combine(int ply, Position &pos,
                      const std::vector<std::pair<CanonKey, Score>> &values,
                      Book &book) {
  const Canon c = canonicalize(pos);
  const CanonKey key = {c.cols, c.occ};
  if (ply == SEARCH_START_PLY)
    return std::lower_bound(values.begin(), values.end(), key,
                            [](const auto &v, const CanonKey &k) {
                              return v.first < k;
                            })
        ->second;
  // Combined already, as this or an equivalent position.
  if (auto it = book.find(key); it != book.end())
    return it->second.value;

  // Same moves as find_jobs, which are the moves negamax searches. Every
  // score is kept, so that every move achieving the best can be listed.
  std::array<std::array<Score, 16>, 16> scores; // [cell][give]
  std::array<Bitboard, 16> safe{};              // gives searched per cell
  // With no safe give, the opponent wins with the next placement.
  Score best = Score(-win_score(std::popcount(pos.occupied()) + 2));
  bool any_safe = false;
  for (Bitboard empty = pos.empty_cells(); empty; empty &= empty - 1) {
    const Cell cell = Cell(std::countr_zero(empty));
    pos.place_piece(cell);
    safe[cell] = pos.available() & ~winning_pieces(pos);
    for (Bitboard s = safe[cell]; s; s &= s - 1) {
      const Piece give = Piece(std::countr_zero(s));
      pos.give_piece(give);
      const Score score = Score(-combine(ply + 1, pos, values, book));
      pos.undo_give_piece();
      scores[cell][give] = score;
      if (!any_safe || score > best)
        best = score;
      any_safe = true;
    }
    pos.undo_place_piece();
  }

  BookEntry e{best, {}};
  for (Bitboard empty = pos.empty_cells(); empty; empty &= empty - 1) {
    const Cell cell = Cell(std::countr_zero(empty));
    // When every move loses at once, every move is listed.
    Bitboard gives = pos.available();
    if (any_safe) {
      gives = 0;
      for (Bitboard s = safe[cell]; s; s &= s - 1)
        if (scores[cell][std::countr_zero(s)] == best)
          gives |= Bitboard(s & -s);
    }
    for (; gives; gives &= gives - 1) {
      const Move m = to_canonical({cell, Piece(std::countr_zero(gives))}, c);
      e.gives[m.cell] |= Bitboard(1u << m.give);
    }
  }
  book[key] = e;
  return best;
}

std::vector<Solver::Job> Solver::create_jobs_() {
  std::vector<Job> jobs;
  std::unordered_set<CanonKey, CanonKeyHash> seen;
  // Every first give is equivalent, so piece 0 in hand covers them all.
  Position pos;
  pos.load(0, {0, 0, 0, 0}, 0);
  find_jobs(0, pos, seen, jobs);
  return jobs;
}

// Adds every position SEARCH_START_PLY turns below `pos` to `jobs`,
// once per group of equivalent positions. `seen` holds the key of
// every position visited so far, at any ply: positions at different plies
// have different piece counts, so they never share one.
//
// Only safe gives are followed, as in negamax: a give that lets the
// opponent complete a line loses at once, so it is never the best move
// unless every give does that, and then the position is a proven loss
// that needs no job. So no position here has a piece in hand that can win,
// and no placement needs a win check.
void Solver::find_jobs(int ply, Position &pos,
                       std::unordered_set<CanonKey, CanonKeyHash> &seen,
                       std::vector<Job> &jobs) {
  Canon c = canonicalize(pos);
  if (!seen.insert({c.cols, c.occ}).second)
    return; // an equivalent position was already expanded or listed
  if (ply == SEARCH_START_PLY) {
    const auto &p = pos.props();
    jobs.push_back({{c.cols, c.occ},
                    std::uint64_t(p[0]) | std::uint64_t(p[1]) << 16 |
                        std::uint64_t(p[2]) << 32 | std::uint64_t(p[3]) << 48,
                    pos.occupied(),
                    pos.in_hand()});
    return;
  }
  for (Bitboard empty = pos.empty_cells(); empty; empty &= empty - 1) {
    pos.place_piece(Cell(std::countr_zero(empty)));
    for (Bitboard safe = pos.available() & ~winning_pieces(pos); safe;
         safe &= safe - 1) {
      pos.give_piece(Piece(std::countr_zero(safe)));
      find_jobs(ply + 1, pos, seen, jobs);
      pos.undo_give_piece();
    }
    pos.undo_place_piece();
  }
}

SolveResult Solver::solve(Position &pos) {
  nodes_ = 0;
  if (pos.empty_cells() == 0)
    return {SOLVED_DRAW, EMPTY_MOVE}; // board full

  const int pieces = std::popcount(pos.occupied());
  if (pos.in_hand() >= 0) {
    // The piece in hand may win outright, or fill the board.
    for (Bitboard e = pos.empty_cells(); e; e &= e - 1) {
      Cell cell = Cell(std::countr_zero(e));
      pos.place_piece(cell);
      bool win = move_wins(pos, cell);
      pos.undo_place_piece();
      if (win)
        return {win_score(pieces + 1), {cell, -1}};
    }
    if (pieces == 15)
      return {SOLVED_DRAW, {Cell(std::countr_zero(pos.empty_cells())), -1}};
  }

  // The result first, then for a win or loss its exact score (see
  // solve.hpp). The windows are open on both sides of every score they can
  // hold, so the score that comes back is exact.
  Move best = EMPTY_MOVE;
  Score value = search_root(pos, -1, 1, best);
  if (!distances_)
    // Fail-hard: a win or loss is only a bound here, so it keeps its sign.
    value = Score(value > 0 ? 1 : value < 0 ? -1 : 0);
  else if (value >= 1)
    value = search_root(pos, Score(PROVEN - 1), WIN, best);
  else if (value <= -1)
    value = search_root(pos, Score(-WIN), Score(1 - PROVEN), best);
  // No move beat a loss at once: all of them lose at once, so any one will do.
  return {value, best != EMPTY_MOVE ? best : any_move(pos)};
}

Score Solver::search_root(Position &pos, Score alpha, Score beta, Move &best) {
  best = EMPTY_MOVE;
  if (pos.in_hand() >= 0)
    return negamax(pos, alpha, beta, &best);

  // Nothing in hand: giving is the only move. Giving a piece that completes
  // a line loses at once, so only safe pieces are searched. On the empty
  // board every piece is equivalent to every other.
  Bitboard safe = pos.occupied() == 0
                      ? Bitboard{1}
                      : Bitboard(pos.available() & ~winning_pieces(pos));
  // No safe piece: the opponent wins with the next placement.
  if (!safe)
    return Score(-win_score(std::popcount(pos.occupied()) + 1));
  for (; safe && alpha < beta; safe &= safe - 1) {
    Piece piece = Piece(std::countr_zero(safe));
    pos.give_piece(piece);
    Score score = Score(-negamax(pos, Score(-beta), Score(-alpha)));
    pos.undo_give_piece();
    if (score > alpha) {
      alpha = score;
      best = {-1, piece};
    }
  }
  return alpha;
}

int mate_distance(Score value, const Position &pos) {
  // A win that leaves n pieces on the board is placed at ply n - first,
  // counted from 0 here, where `first` is the piece count after the first
  // placement from here; even plies are the side to move's. With nothing in
  // hand the side to move only gives, so the opponent places at ply 1.
  const int first =
      std::popcount(pos.occupied()) + (pos.in_hand() >= 0 ? 1 : 0);
  if (value >= PROVEN)
    return (WIN - value - first) / 2 + 1;
  if (value <= -PROVEN)
    return -((WIN + value - first + 1) / 2);
  return 0;
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

  const bool canon = pieces <= canon_max_pieces_;
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

  // The TT move comes first: its cell before the other cells, and its give
  // first on that cell. It may come from an equivalent position or, very
  // rarely, a hash collision, so its cell is only taken if empty, and
  // pop_first only takes its give if available and safe. The other cells
  // follow by cell_priors, computed only once the TT cell has not cut; the
  // pieces in index order. Unsafe gives are never searched: they would break
  // the invariant that the piece in hand cannot win.
  Bitboard empty = pos.empty_cells();
  std::array<int, 16> prior;
  bool have_prior = false;
  while (empty) {
    Cell cell;
    if (entry_move.cell >= 0 && empty >> entry_move.cell & 1) {
      cell = entry_move.cell;
    } else {
      if (!have_prior) {
        prior = cell_priors(pos);
        have_prior = true;
      }
      cell = Cell(lowest_prior(empty, prior));
    }
    empty &= Bitboard(~(Bitboard{1} << cell));
    pos.place_piece(cell);
    Bitboard avail = pos.available() & ~winning_pieces(pos);
    if (avail)
      any_safe = true;
    while (avail) {
      Piece give = Piece(
          pop_first(avail, cell == entry_move.cell ? entry_move.give : -1));
      pos.give_piece(give);
      Score score = -negamax(pos, -beta, -alpha);
      pos.undo_give_piece();
      if (score > alpha) {
        alpha = score;
        best_move = {cell, give};
      }
      if (alpha >= beta) {
        pos.undo_place_piece();
        if (!stopped())
          tt_.store(key, alpha, depth, to_tt(best_move), Bound::Lower);
        if (best_out)
          *best_out = best_move;
        return alpha;
      }
    }
    pos.undo_place_piece();
  }

  // Every give, after every placement, lets the opponent win with the
  // next piece.
  if (!any_safe) {
    const Score loss = Score(-win_score(pieces + 2));
    tt_.store(key, loss, depth, EMPTY_MOVE, Bound::Exact);
    return loss;
  }
  if (!stopped())
    tt_.store(key, alpha, depth, to_tt(best_move),
              alpha > alpha_orig ? Bound::Exact : Bound::Upper);
  if (best_out)
    *best_out = best_move;
  return alpha;
}

} // namespace quarto
