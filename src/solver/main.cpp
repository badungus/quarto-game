// quarto_solve: proves whether the side to move wins, draws or loses.
//
//   quarto_solve [position] [--tt MB] [--canon N]
//
// `position` uses the UQI format, e.g. ChSfchsf../ChSF..chSF/..../...chsF|chSF;
// without one it solves the empty board. --tt sets the transposition table
// size in MB (default 8192), --canon the most pieces a position may have to be
// keyed by its canonical form (default Solver::DEFAULT_CANON_MAX_PIECES, -1
// for never).

#include "position.hpp"
#include "solve.hpp"
#include "tt.hpp"
#include "uqi.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {
constexpr const char *EMPTY_BOARD = "..../..../..../....|-";
// The TT rounds its size down to a power of two, so on a 32 GB machine the
// choice is 8 GB or 16 GB. 8 GB leaves room for the OS and other programs;
// 16 GB only pays off if the rest stays well under 16 GB, since swapping
// slows a solve far more than a smaller table does.
constexpr std::size_t DEFAULT_TT_MB = 8192;

const char *outcome(quarto::Score value) {
  if (value == quarto::SOLVED_WIN)
    return "win";
  if (value == quarto::SOLVED_LOSS)
    return "loss";
  return "draw";
}
} // namespace

int main(int argc, char **argv) {
  std::string position = EMPTY_BOARD;
  std::size_t tt_mb = DEFAULT_TT_MB;
  int canon = quarto::Solver::DEFAULT_CANON_MAX_PIECES;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--tt" && i + 1 < argc) {
      tt_mb = std::strtoull(argv[++i], nullptr, 10);
    } else if (arg == "--canon" && i + 1 < argc) {
      canon = std::atoi(argv[++i]);
    } else if (arg.find('|') != std::string::npos) {
      position = arg;
    } else {
      std::fprintf(stderr,
                   "usage: quarto_solve [position] [--tt MB] [--canon N]\n");
      return 1;
    }
  }

  auto [occ, prop, hand] = quarto::parse_position(position);
  quarto::Position pos;
  pos.load(occ, prop, hand);

  quarto::TranspositionTable tt(tt_mb);
  quarto::Solver solver(tt);
  solver.set_canon_max_pieces(canon);

  const auto start = std::chrono::steady_clock::now();
  auto seconds = [&] {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                         start)
        .count();
  };
  solver.set_progress([&](std::uint64_t nodes) {
    std::fprintf(stderr, "  %llu nodes, %.0f s\n", (unsigned long long)nodes,
                 seconds());
  });

  std::printf("position: %s\ntt:       %zu MB\ncanon:    <= %d pieces\n",
              position.c_str(), tt_mb, canon);
  quarto::SolveResult result = solver.solve(pos);
  const double s = seconds();

  std::printf("result:   %s for the side to move\n", outcome(result.value));
  std::printf("move:     %s\n",
              quarto::format_move(result.move,
                                  result.value == quarto::SOLVED_WIN)
                  .c_str());
  std::printf("nodes:    %llu\ntime:     %.2f s (%.1f M nodes/s)\n",
              (unsigned long long)solver.nodes(), s,
              s > 0 ? solver.nodes() / s / 1e6 : 0.0);
  return 0;
}
