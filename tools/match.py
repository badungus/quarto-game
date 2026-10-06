"""Headless UQI match runner: plays games between two engines from the start
position, alternating who moves first, and checks every move itself.

    python tools/match.py --a thor --b odin --games 100 --movetime 200

Engines are looked up in build/ (change with --build). Every game prints the
seeds both engines got, so a game at fixed depth can be replayed.
"""
import argparse
import math
import os
import random
import subprocess

LINES = [0x000F, 0x00F0, 0x0F00, 0xF000, 0x1111, 0x2222, 0x4444, 0x8888, 0x8421, 0x1248]
BUILD = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build")


def name(p):
    return "".join(("SCHF" if p >> i & 1 else "schf")[i] for i in range(4))


def parse_piece(s):
    return sum(1 << i for i in range(4) if s[i].isupper())


def fmt(board, hand):
    ranks = ["".join("." if p < 0 else name(p) for p in board[r * 4:r * 4 + 4]) for r in range(4)]
    return "/".join(ranks) + "|" + ("-" if hand < 0 else name(hand))


def won(board):
    for line in LINES:
        ps = [board[c] for c in range(16) if line >> c & 1]
        if -1 in ps:
            continue
        a, o = 15, 0
        for p in ps:
            a &= p
            o |= p
        if a or o != 15:
            return True
    return False


class Engine:
    def __init__(self, exe, seed):
        self.p = subprocess.Popen([exe], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, text=True)
        self.send(f"seed {seed}")

    def send(self, line):
        self.p.stdin.write(line + "\n")
        self.p.stdin.flush()

    def go(self, pos, movetime):
        self.send(f"go movetime {movetime} position {pos}")
        info = ""
        while True:
            line = self.p.stdout.readline()
            if not line:
                raise RuntimeError("engine exited")
            if line.startswith("info depth"):
                info = line.strip()
            elif line.startswith("bestmove"):
                return line.split()[1], info

    def quit(self):
        self.send("quit")
        self.p.wait(timeout=5)


def play(exes, seeds, movetime):
    """Returns (winner index or None for a draw, number of placements, reason)."""
    engines = [Engine(e, s) for e, s in zip(exes, seeds)]
    board, hand, side = [-1] * 16, -1, 0
    try:
        while True:
            move, _ = engines[side].go(fmt(board, hand), movetime)
            cell_s, give_s = move.split("|")
            if cell_s != "-":
                cell = 4 * (4 - int(cell_s[1])) + ord(cell_s[0]) - ord("a")
                if hand < 0 or board[cell] >= 0:
                    return 1 - side, 16 - board.count(-1), f"illegal placement {move}"
                board[cell], hand = hand, -1
                if won(board):
                    return side, 16 - board.count(-1), "win"
                if -1 not in board:
                    return None, 16, "board full"
            elif hand >= 0:
                return 1 - side, 16 - board.count(-1), f"no placement {move}"
            if give_s in ("-", "#"):
                return 1 - side, 16 - board.count(-1), f"no give {move}"
            give = parse_piece(give_s)
            if give in board:
                return 1 - side, 16 - board.count(-1), f"illegal give {move}"
            hand, side = give, 1 - side
    finally:
        for e in engines:
            e.quit()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--a", default="thor", help="first engine (executable name)")
    ap.add_argument("--b", default="odin", help="second engine (executable name)")
    ap.add_argument("--games", type=int, default=100)
    ap.add_argument("--movetime", type=int, default=200, help="ms per move")
    ap.add_argument("--seed", type=int, default=1, help="seeds the engine seeds")
    ap.add_argument("--build", default=BUILD, help="directory with the engines")
    args = ap.parse_args()

    if os.name == "nt":
        args.a, args.b = (e if e.endswith(".exe") else e + ".exe" for e in (args.a, args.b))

    rng = random.Random(args.seed)
    score = {args.a: 0, args.b: 0, "draw": 0}
    first_wins = second_wins = 0
    lengths = []
    for g in range(args.games):
        exes = [args.a, args.b] if g % 2 == 0 else [args.b, args.a]
        seeds = [rng.randrange(2**32), rng.randrange(2**32)]
        winner, length, reason = play([os.path.join(args.build, e) for e in exes],
                                      seeds, args.movetime)
        lengths.append(length)
        if winner is None:
            score["draw"] += 1
            result = "draw"
        else:
            score[exes[winner]] += 1
            first_wins += winner == 0
            second_wins += winner == 1
            result = f"{exes[winner]} wins"
        print(f"game {g + 1:3}: {exes[0]} first, seeds {seeds}, {result} after "
              f"{length} placements ({reason})", flush=True)

    n = args.games
    a, d = score[args.a], score["draw"]
    p = (a + d / 2) / n
    print(f"\n{args.a} {score[args.a]} - {args.b} {score[args.b]} - draws {d}  "
          f"({args.a} scores {100 * p:.1f}%)")
    print(f"first player wins {first_wins}, second player wins {second_wins}, "
          f"average length {sum(lengths) / n:.1f} placements")
    if 0 < p < 1:
        elo = -400 * math.log10(1 / p - 1)
        # Standard error of the score from the per-game results.
        var = (a * (1 - p) ** 2 + d * (0.5 - p) ** 2 + score[args.b] * p ** 2) / n
        se = math.sqrt(var / n)
        lo, hi = (max(1e-9, min(1 - 1e-9, x)) for x in (p - 1.96 * se, p + 1.96 * se))
        elo_lo, elo_hi = (-400 * math.log10(1 / x - 1) for x in (lo, hi))
        print(f"Elo {args.a} vs {args.b}: {elo:+.0f}  (95% CI {elo_lo:+.0f} .. {elo_hi:+.0f})")


if __name__ == "__main__":
    main()
