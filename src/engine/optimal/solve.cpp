#include "solve.hpp"
#include "canonical.hpp"
#include "ordering.hpp"
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
} // namespace

// The jobs are the positions with this many pieces placed.
// The book holds every position before them.
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
    std::fclose(in);
    return book; // not a book file
  }
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
  // Jobs are solved in the order they were found, so jobs next to each other
  // share more TT entries.
  const std::vector<Job> jobs = create_jobs_();
  // Job indices sorted by key, to find a job by binary search.
  std::vector<std::uint32_t> by_key(jobs.size());
  std::iota(by_key.begin(), by_key.end(), 0u);
  std::sort(by_key.begin(), by_key.end(),
            [&](std::uint32_t a, std::uint32_t b) {
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
    return std::lower_bound(
               values.begin(), values.end(), key,
               [](const auto &v, const CanonKey &k) { return v.first < k; })
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

// Adds every position SEARCH_START_PLY turns below pos to jobs, once for
// each group of equivalent positions. Only safe gives are followed.
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

  // First only win, draw or loss, then the exact score of a win or loss.
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

  // Nothing in hand, so only safe gives are searched. On the empty board
  // every piece is the same, so only one is searched.
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
  // first is the number of pieces after the next placement. Even plies
  // belong to the side to move.
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
  // The key holds the whole position, so TT matches are exact.
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
  // Depth never limits a cutoff here. It is stored so the TT keeps the
  // bigger subtrees.
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
  // The return value of a node that was cut off.
  auto cutoff = [&] {
    if (!stopped())
      tt_.store(key, alpha, depth, to_tt(best_move), Bound::Lower);
    if (best_out)
      *best_out = best_move;
    return alpha;
  };

  // The TT move alone first. Unsafe gives are never searched.
  const bool tt_move_legal =
      entry_move.cell >= 0 && pos.empty_cells() >> entry_move.cell & 1 &&
      entry_move.give >= 0 && pos.available() >> entry_move.give & 1;
  if (tt_move_legal) {
    pos.place_piece(entry_move.cell);
    if ((pos.available() & ~winning_pieces(pos)) >> entry_move.give & 1) {
      any_safe = true;
      if (search_give(entry_move.cell, entry_move.give)) {
        pos.undo_place_piece();
        return cutoff();
      }
    }
    pos.undo_place_piece();
  }

  // Then every cell by prior, with the gives in index order.
  Bitboard empty = pos.empty_cells();
  std::array<Score, 16> prior = compute_prior(pos);
  while (empty) {
    Cell cell = best_cell(empty, prior);
    pos.place_piece(cell);
    Bitboard avail = pos.available() & ~winning_pieces(pos);
    if (avail)
      any_safe = true;
    // The TT move was searched already.
    if (tt_move_legal && entry_move.cell == cell)
      avail &= Bitboard(~(Bitboard{1} << entry_move.give));
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
