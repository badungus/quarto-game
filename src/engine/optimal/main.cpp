// optimal: an engine that only plays proven moves.
//
//   optimal                                    UQI engine, as the GUI starts it
//   optimal --solve POSITION [--tt MB] [--no-distance]
//                                              solve one position
//   optimal --full [--threads N] [--tt MB] [--no-distance]
//                                              solve the game, write the book
//
// The engine and --full use book.bin in the source directory of optimal (set
// by CMake, so it is versioned and survives a clean build), or
// square_book.bin with --squares. The engine plays from it while the
// position is in it, picking at random among the best moves, and solves to
// the end of the game after that. --full solves the empty board on N threads
// (default: one per hardware thread) and writes every position of the first
// six turns to the book, with all its best moves. It takes hours; finished
// jobs are saved to book.bin.jobs as they finish, so an interrupted run picks
// up where it stopped.
//
// --solve proves whether the side to move wins, draws or loses in POSITION
// (UQI format, e.g. ChSfchsf../ChSF..chSF/..../...chsF|chSF) on one thread,
// without the book, and prints how soon (mate, as in UQI), a move achieving
// it, the nodes and the time.
// --tt is the TT size in MB for both (default 8192).
// --no-distance proves only win, draw or loss, not how soon, for both. That
// skips the second search of every win and loss (see solve.hpp), so it is
// faster; a book written with it has no mate distances.

#include "engine.hpp"
#include "position.hpp"
#include "solve.hpp"
#include "tables.hpp"
#include "tt.hpp"
#include "uqi.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>

int main(int argc, char **argv) {

  bool full = false;
  std::string position; // set by --solve
  unsigned threads = std::thread::hardware_concurrency();
  std::size_t tt_mb = 8192;
  bool distances = true;
  bool bad_args = false;
  for (int i = 1; i < argc && !bad_args; ++i) {
    std::string arg = argv[i];
    if (arg == "--full") {
      full = true;
    } else if (arg == "--squares") {
      quarto::squares_win = true;
    } else if (arg == "--solve" && i + 1 < argc) {
      position = argv[++i];
      bad_args = !quarto::valid_position(position);
    } else if (arg == "--threads" && i + 1 < argc) {
      threads = unsigned(std::strtoul(argv[++i], nullptr, 10));
    } else if (arg == "--tt" && i + 1 < argc) {
      tt_mb = std::strtoull(argv[++i], nullptr, 10);
    } else if (arg == "--no-distance") {
      distances = false;
    } else {
      bad_args = true;
    }
  }
  // --no-distance only means something to a solve, not to the engine.
  if (bad_args || (full && !position.empty()) ||
      (!distances && !full && position.empty())) {
    std::fprintf(stderr,
                 "usage: optimal\n"
                 "       optimal --solve POSITION [--tt MB] [--no-distance]\n"
                 "       optimal --full [--threads N] [--tt MB] "
                 "[--no-distance]\n");
    return 1;
  }
  const std::string book = quarto::squares_win
                               ? OPTIMAL_SQUARES_BOOK_PATH
                               : OPTIMAL_BOOK_PATH; // set by CMake

  if (!full && position.empty()) {
    // Until --full has written the book, the older text book next to it
    // (book.txt for book.bin) stands in.
    std::string engine_book = book;
    if (!std::filesystem::exists(engine_book))
      engine_book =
          std::filesystem::path(book).replace_extension(".txt").string();
    quarto::Engine engine(engine_book);
    quarto::uqi_loop(engine);
    return 0;
  }

  quarto::TranspositionTable tt(tt_mb);
  quarto::Solver solver(tt);
  solver.set_distances(distances);
  if (full)
    std::printf("book:     %s\ntt:       %zu MB\nthreads:  %u\n", book.c_str(),
                tt_mb, threads);
  else
    std::printf("position: %s\ntt:       %zu MB\n", position.c_str(), tt_mb);
  std::fflush(stdout);

  const auto start = std::chrono::steady_clock::now();
  // The solved position: the empty board with nothing in hand for --full.
  quarto::Position pos;
  if (full) {
    pos.load(0, {0, 0, 0, 0}, -1);
  } else {
    auto [occ, prop, hand] = quarto::parse_position(position);
    pos.load(occ, prop, hand);
  }
  quarto::SolveResult result =
      full ? solver.full_solve(threads, book) : solver.solve(pos);
  const double s =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
          .count();

  if (full) {
    std::printf("result:   %s for the first player (who gives first)\n",
                quarto::outcome(result.value));
  } else {
    std::printf("result:   %s for the side to move\n",
                quarto::outcome(result.value));
  }
  if (int mate = quarto::mate_distance(result.value, pos))
    std::printf("mate:     %d\n", mate);
  if (!full)
    std::printf("move:     %s\n",
                quarto::format_move(result.move, result.value > 0).c_str());
  std::printf("nodes:    %llu\ntime:     %.2f s (%.1f M nodes/s)\n",
              (unsigned long long)solver.nodes(), s,
              s > 0 ? solver.nodes() / s / 1e6 : 0.0);
  return 0;
}
