# Symmetry reduction for Odin

This document describes how to add symmetry reduction to Odin, and how to use
it to solve Quarto from the empty board, as Koolen did. Koolen's solver has
three parts:

| Technique                 | Odin today                           |
| ------------------------- | ------------------------------------ |
| Alpha-beta pruning        | yes (`Search::negamax`)              |
| Memoisation               | yes (`TranspositionTable`)           |
| Symmetry reduction        | **missing**                          |

Everything below is additive. `Position`, the move generator, the win checks
and the scoring scheme stay the same. Symmetry changes three things:

1. **What the TT is keyed by.** Equivalent positions must share one key.
2. **How TT moves are stored.** They are stored in the canonical frame and
   mapped back on probe.
3. **Which moves are generated.** In a position that is symmetric with
   itself, only one move of each group of equivalent moves is generated.

New files: `canonical.hpp` and `canonical.cpp` (add `canonical.cpp` to
`odin_core` in `CMakeLists.txt`) with the symmetry tables in them: only canonicalisation uses those, so they stay out of `tables.hpp`.

---

## 1. Why the scores already work

A proven win scores `WIN - pieces_on_board` (`win_score`), and a draw or an
unproven position scores 0. A symmetry never changes the piece count, and
`eval` only asks whether every placement and give loses, which symmetry does
not change either. So equivalent positions have **identical scores at
identical depths**, and a TT entry from one position is valid for every
position equivalent to it. Nothing in the scoring needs to change.

---

## 2. The symmetry group

A Quarto position is unchanged in value by two independent kinds of
transformation:

- **Board symmetries (32):** cell permutations that map the 10 win lines
  (`LINES`) onto themselves.
- **Piece relabelings (384 = 4! × 2⁴):** reorder the four properties, and
  invert any subset of them (tall ↔ short, and so on). A relabeling is applied
  to every piece at once: the pieces on the board, the piece in hand and the
  pieces still available.

The two kinds commute, so the full group has 32 × 384 = 12,288 elements.

> This assumes the win lines are only rows, columns and the two diagonals,
> which is what `LINES` has. If 2×2 squares ever become a win condition, the
> board group shrinks and the table below must be rebuilt.

### 2.1 The 32 board symmetries

Write a cell as `(r, c)` with `cell = r * 4 + c`. Take a permutation σ of
`{0,1,2,3}` that commutes with reversal, meaning `σ(3 - i) = 3 - σ(i)`. Exactly
8 such permutations exist:

```
0123  0213  1032  1302  2031  2301  3120  3210
```

`0213` is the "mid flip" (swap the two middle rows and columns). `1032` is
"inside out" (swap the outer pair with the inner pair). `3210` is a mirror.

Each board symmetry is:

```
(r, c)  ->  (σ(r), τ(c)),  then optionally transpose
```

where τ is either σ or `3 - σ`. That gives 8 × 2 × 2 = 32 symmetries. Rows go
to rows and columns go to columns (or the other way round after transposing).
If τ = σ, the main diagonal `(i, i)` stays on itself; if τ = `3 - σ`, the two
diagonals swap. So every line maps to a line.

```cpp
// canonical.hpp (public, so tests can check them)

// Permutations of a row/column index that commute with i -> 3 - i. Applied to
// rows and columns together they keep both diagonals diagonals.
inline constexpr std::array<std::array<int, 4>, 8> AXIS_PERMS = {{
    {0, 1, 2, 3}, {0, 2, 1, 3}, {1, 0, 3, 2}, {1, 3, 0, 2},
    {2, 0, 3, 1}, {2, 3, 0, 1}, {3, 1, 2, 0}, {3, 2, 1, 0},
}};

constexpr int SYM_COUNT = 32;

// SYMS[s][cell] is where symmetry s sends cell. SYMS[0] is the identity.
constexpr std::array<std::array<Cell, 16>, SYM_COUNT> make_syms() {
  std::array<std::array<Cell, 16>, SYM_COUNT> syms{};
  int n = 0;
  for (const auto &sigma : AXIS_PERMS) {
    for (int mirror = 0; mirror < 2; ++mirror) {
      for (int transpose = 0; transpose < 2; ++transpose) {
        for (int cell = 0; cell < 16; ++cell) {
          int r = sigma[cell / 4];
          int c = sigma[cell % 4];
          if (mirror)
            c = 3 - c;
          if (transpose)
            std::swap(r, c);
          syms[n][cell] = Cell(r * 4 + c);
        }
        ++n;
      }
    }
  }
  return syms;
}

inline constexpr auto SYMS = make_syms();

// INV_SYMS[s] undoes SYMS[s].
constexpr std::array<std::array<Cell, 16>, SYM_COUNT> make_inv_syms() {
  std::array<std::array<Cell, 16>, SYM_COUNT> inv{};
  for (int s = 0; s < SYM_COUNT; ++s)
    for (int cell = 0; cell < 16; ++cell)
      inv[s][SYMS[s][cell]] = Cell(cell);
  return inv;
}

inline constexpr auto INV_SYMS = make_inv_syms();
```

### 2.2 Applying a symmetry to a bitboard

Canonicalisation permutes every bitboard 32 times per call, so it should be
fast. Split the bitboard into two bytes and look each one up:

```cpp
// canonical.cpp, anonymous namespace

// SYM_BITS[s].lo[b] is the low byte b with every bit moved by SYMS[s];
// .hi the same for the high byte. 32 KB in total.
struct SymBits {
  std::array<Bitboard, 256> lo{};
  std::array<Bitboard, 256> hi{};
};

constexpr std::array<SymBits, SYM_COUNT> make_sym_bits() {
  std::array<SymBits, SYM_COUNT> t{};
  for (int s = 0; s < SYM_COUNT; ++s)
    for (int b = 0; b < 256; ++b)
      for (int bit = 0; bit < 8; ++bit)
        if ((b >> bit) & 1) {
          t[s].lo[b] |= Bitboard(1u << SYMS[s][bit]);
          t[s].hi[b] |= Bitboard(1u << SYMS[s][bit + 8]);
        }
  return t;
}

inline constexpr auto SYM_BITS = make_sym_bits();

constexpr Bitboard apply_sym(int s, Bitboard b) {
  return Bitboard(SYM_BITS[s].lo[b & 0xFF] | SYM_BITS[s].hi[b >> 8]);
}
```

`make_syms` uses `std::swap`, so `canonical.hpp` needs `#include <utility>`.
GCC's default constexpr operation limit is far above what these tables need.
If a compiler complains, build them in a function-local `static` at startup
instead.

---

## 3. Canonical form

### 3.1 The piece in hand

The piece in hand is part of the state and must be relabeled along with the
board, but it stays a separate `Piece`, as in `Position`. Board symmetries
don't move it. The available pieces are fully determined by `occ`, `prop` and
`hand`, so they need no separate handling.

### 3.2 Normalising the piece relabeling (no 384-way loop)

Each `prop[i]` describes one property across all occupied cells, as a column.
A relabeling does two things to these four columns:

- **Inverting property i** replaces `prop[i]` with `prop[i] ^ occ`, and
  flips bit i of the piece in hand.
- **Reordering properties** reorders the four columns, and the bits of the
  piece in hand.

So the relabeling can be normalised in two steps, without trying all 384:

1. **Fix the inversions.**
   - With a piece in hand, invert exactly the properties the hand has
     (`flip = hand`). The hand becomes piece 0 in every canonical form, so it
     doesn't need to be stored at all.
   - With no piece in hand (game start, or a loaded give-only position), take
     `q[i] = min(prop[i], prop[i] ^ occ)`. Inverting property i doesn't change
     `q[i]`.
2. **Sort `q[0..3]` ascending.** Reordering properties doesn't change the
   sorted list. With a piece in hand, the hand is 0000 and stays 0000 under any
   reordering.

The sorted list `q` is a **complete invariant** under relabeling. Two
positions with the same board are related by a relabeling exactly when their
sorted `q` match (and both have a piece in hand, or both don't).

### 3.3 Normalising the board symmetry

For each of the 32 symmetries s, permute `occ` and the four `prop` bitboards
with `apply_sym(s, ·)` and then apply §3.2. That gives a tuple
`(occ', q0, q1, q2, q3)`. The canonical form is the **lexicographically
smallest** of the 32 tuples. Board symmetries and relabelings commute, and the
32 symmetries form a group, so every position in an equivalence class produces
the same set of 32 tuples, and therefore the same minimum.

**Fast path.** The tuple compares `occ'` first, and `occ'` depends only on
`occ`. Compute `apply_sym(s, occ)` for all 32 symmetries first, and keep only
the ones that reach the smallest value. Usually only one or a few do, so the
expensive work in §3.2 runs a handful of times instead of 32.

### 3.4 Packing and hashing

All the candidates share the same `occ'`, so only the columns need comparing.
The four 16-bit columns pack into one word:

```
cols = q0 | q1 << 16 | q2 << 32 | q3 << 48   // 64 bits, compared directly
```

`Canon` keeps `cols`, the occupancy `occ'` and a has-hand flag. Comparing
`cols` is exact, and `stabilizer` relies on that.

The TT key is not a hash but the canonical position itself: a `TTKey` of
`{cols, occ'}`. The occupancy has to be part of it, because a cell holding
piece 0 and an empty cell both read as 0 in every column. In canonical form
the hand is piece 0, and the available pieces are the ones neither placed nor
in hand, so these 80 bits are the whole position. The has-hand flag is left
out: the TT only holds positions with a piece in hand (§4.1). The TT compares
the stored key with the probed one, so a TT match is always the same
position, never a collision. That matters for a proof.

### 3.5 Remembering the transformation

The TT move must be mapped between the real frame and the canonical frame
(§4), so canonicalisation also returns the transformation it used. The same
file builds the stabilizer that §5 uses to skip moves.

The implementation is in `src/engine/odin/canonical.hpp` and
`canonical.cpp`. The public interface:

```cpp
struct Canon {
  std::uint64_t cols;                // sorted property columns, 16 bits each
  Bitboard occ;                      // occupancy after `sym`
  bool has_hand;                     // in canonical form the hand is piece 0
  std::uint8_t sym;                  // board symmetry, index into SYMS
  std::array<std::uint8_t, 4> order; // canonical property j = original order[j]
  std::uint8_t flip;                 // bit i: original property i is inverted
  bool symmetric; // some non-identity transformation fixes the position

  Bitboard col(int j) const;         // column j of cols
};

struct Transform { // where each cell goes and what each piece becomes
  std::array<Cell, 16> cell;
  std::array<Piece, 16> piece;
};

Canon canonicalize(const Position &pos);
Move to_canonical(Move m, const Canon &c);
Move from_canonical(Move m, const Canon &c);
std::vector<Transform> stabilizer(const Position &pos, const Canon &c);
```

Inside `canonical.cpp`:

- `occ_candidates` runs the fast path from §3.3.
- `normalise` runs §3.2 for one symmetry and returns a `Canon` for it.
- `canonicalize` keeps the one with the smallest `cols` and counts ties for
  `symmetric`. The same struct serves both, so there is no separate
  per-symmetry type.
- `stabilizer` is explained in §5.1.

Notes on the relabeling direction. Canonical property `j` is original property
`i = order[j]`, inverted if `flip` bit `i` is set. So a piece's canonical bit
`j` is `original bit i XOR flip bit i`, and `unrelabel` reverses that bit by
bit.

`stabilizer` matches symmetries by comparing `cols` directly. Every candidate
has the same occupancy and hand, so that comparison is exact, with no hash
involved.

If two transformations tie, it doesn't matter which one is returned. Both
produce the same canonical position, so the inverse of either one maps a
canonical move to a legal, equivalent real move.

---

## 4. Wiring it into the TT

### 4.1 When to canonicalise

Canonicalising costs a few hundred operations; reading the position as it is
costs almost nothing. Symmetry pays off most near the root, where many
positions are equivalent and subtrees are huge. Deep in the tree, positions
rarely coincide and subtrees are cheap. So key positions by their canonical
form only up to a piece count, and by the position as it is after that:

```cpp
// search.cpp
// Positions with at most this many pieces on the board are keyed by their
// canonical form; deeper ones by the position as it is. Tune by benchmark
// (section 7).
constexpr int CANON_MAX_PIECES = 10;
```

The position as it is: the four property bitboards packed into 64 bits, and
`occ | hand << 16`. Both kinds of key are exact, and both can share one table:
the piece count decides which kind a position gets, so the same position
always gets the same kind of key, and keys of the two kinds never meet.

Neither kind needs a has-hand bit. `negamax` is only ever called right after a
give, so every position in the TT has a piece in hand. The give-only opening
(nothing in hand) is handled at the root, which does not use the TT.

### 4.2 Changes to `Search::negamax`

```cpp
Score Search::negamax(Position &pos, int depth, Score alpha, Score beta) {
  ++nodes_;
  // ... movetime check unchanged ...

  const bool canon = std::popcount(pos.occupied()) <= CANON_MAX_PIECES;
  Canon c{};
  TTKey key;
  if (canon) {
    c = canonicalize(pos);
    key = {c.cols, c.occ};
  } else {
    const auto &p = pos.props();
    key = {std::uint64_t(p[0]) | std::uint64_t(p[1]) << 16 |
               std::uint64_t(p[2]) << 32 | std::uint64_t(p[3]) << 48,
           std::uint32_t(pos.occupied() | pos.in_hand() << 16)};
  }
  // Stored moves are in the frame of the key: canonical or real.
  auto to_tt = [&](Move m) { return canon ? to_canonical(m, c) : m; };

  const std::optional<TTHit> entry = tt_.probe(key);
  Move entry_move = EMPTY_MOVE;
  if (entry)
    entry_move = canon ? from_canonical(entry->move, c) : entry->move;
  // ... bound checks unchanged ...

  // ... search loop unchanged, except every store goes through to_tt:
  //   tt_.store(key, alpha, depth, to_tt({cell, piece}), Bound::Lower);
  //   tt_.store(key, alpha, depth, to_tt(best_move), ...);
  // The `!any_safe` store passes EMPTY_MOVE, which to_tt leaves alone.
}
```

**Don't skip the move mapping.** If you store real-frame moves under a
canonical key, ordering at every equivalent position will try the TT move's
cell and piece first even though they aren't the best move there. The results
stay correct but the search gets much slower, and that is hard to notice.

---

## 5. Skipping equivalent moves during generation

The TT already merges equivalent subtrees *after* the first one is searched.
In a position that maps onto itself, though, equivalent children are
siblings, and the TT entry may not cut the second one off (wrong bound, or
replaced). It is cleaner not to generate them at all. This can be decided
from the move itself, without making it.

### 5.1 The stabilizer

The **stabilizer** of `P` is the set of transformations `T` (board symmetry
plus relabeling) with `T(P) = P`. For any such `T` and any move
`m = (cell, give)`:

```
T(P + m) = T(P) + T(m) = P + T(m),    T(m) = (T.cell[cell], T.piece[give])
```

So `m` and `T(m)` lead to equivalent children. `T` fixes the piece in hand
(it is part of the position), so the placed piece stays the same piece. `T` also maps
empty cells to empty cells and available pieces to available pieces, so
`T(m)` is always legal. And `T(m)` is safe exactly when `m` is, because
equivalent children score the same.

The rule:

> Search `m` only if no `T` in the stabilizer maps it to a smaller move,
> comparing moves as `(cell, give)`.

Every group of equivalent moves keeps exactly one move, the smallest, so the
child scores still reach the same maximum. That makes the skip safe in
fail-hard alpha-beta, at every depth (equivalent children have equal
depth-limited values too, because `eval` is symmetric), and in any search
order.

`stabilizer()` in §3.5 builds the stabilizer. Suppose `T0, T1, …` all map `P`
to its canonical form. Then `T0⁻¹ ∘ Tk` maps `P` onto itself, and every
element of the stabilizer arises this way. The `Tk` are all combinations of:

- a board symmetry that reaches the minimum tuple;
- a reordering of equal property columns.

Usually the stabilizer is just the identity. `c.symmetric` is false then, and
nothing below runs.

### 5.2 Splitting the rule into cells and gives

The rule splits into two cheap masks:

- **Cells:** if some `T` maps cell `c` to a smaller cell, every move on `c`
  has a smaller equivalent, so skip `c` entirely.
- **Gives:** on a cell that is kept, `c` is the smallest in its group, so
  `T(m) < m` needs `T.cell[c] == c` and `T.piece[give] < give`. Only the
  elements that fix `c` matter.

```cpp
// search.cpp, anonymous namespace

// Cells some self-symmetry sends to a smaller cell. Every move on them has a
// smaller equivalent.
Bitboard skipped_cells(const std::vector<Transform> &stab) {
  Bitboard skip = 0;
  for (const Transform &t : stab)
    for (int c = 0; c < 16; ++c)
      if (t.cell[c] < c)
        skip |= Bitboard(1u << c);
  return skip;
}

// Gives after placing on `cell` that a self-symmetry fixing `cell` sends to a
// smaller piece.
Bitboard skipped_gives(const std::vector<Transform> &stab, Cell cell) {
  Bitboard skip = 0;
  for (const Transform &t : stab)
    if (t.cell[cell] == cell)
      for (int p = 0; p < 16; ++p)
        if (t.piece[p] < p)
          skip |= Bitboard(1u << p);
  return skip;
}

// The smallest move equivalent to m, by (cell, give). A -1 cell (give-only
// move at the root) is left as is and only the give is reduced.
Move representative(Move m, const std::vector<Transform> &stab) {
  Move best = m;
  if (m.give < 0)
    return best;
  for (const Transform &t : stab) {
    Move image = {m.cell < 0 ? m.cell : t.cell[m.cell], t.piece[m.give]};
    if (image.cell < best.cell ||
        (image.cell == best.cell && image.give < best.give))
      best = image;
  }
  return best;
}
```

In `negamax`, after the TT probe:

```cpp
std::vector<Transform> stab;
if (canon && c.symmetric)
  stab = stabilizer(pos, c);
// The TT move may be a skipped move. Order by the equivalent move that is
// searched instead.
if (!stab.empty())
  entry_move = representative(entry_move, stab);

Bitboard empty = pos.empty_cells();
if (!stab.empty())
  empty &= ~skipped_cells(stab);
while (empty) {
  Cell cell = pop_first(empty, entry_move.cell);
  pos.place_piece(cell);
  Bitboard avail = pos.available() & ~winning_pieces(pos);
  if (!stab.empty())
    avail &= ~skipped_gives(stab, cell);
  // ... rest unchanged ...
}
```

`any_safe` stays correct. If a skipped give is safe, so is the smaller move it
is equivalent to, and that move is searched. So the masks never empty `avail`
when a safe give exists.

`representative` leaves `EMPTY_MOVE` alone (its `give` is -1), so a missing TT
move needs no special case.

### 5.3 What this does not catch

Two children can be equivalent through a `T` that does **not** fix the
parent. For example, the placed piece maps onto a piece already on the board
and that piece maps onto the placed one. That `T` is not in the stabilizer, so
the rule above can't see it. Those children are ordinary transpositions: the
second one finds the first one's entry under the canonical TT key. Catching
them at generation time would need a canonicalisation for every child, which
costs more than the TT probe it saves.

### 5.4 The root

`negamax_root` always canonicalises, whatever `CANON_MAX_PIECES` is set to,
and uses the same masks in its placement loop. Two more details:

- **The very first move** (empty board, nothing in hand).
  `stabilizer()` does not enumerate it, because every relabeling fixes that
  position. All 16 gives are equivalent, so search piece 0 only.
- **`first`** (the previous iteration's pick) can be a skipped move, because
  it was chosen from the expanded ties below. Pass it through
  `representative` before ordering.

The root picks a random move among the moves tied for best. Skipped moves
score the same as their representative, so add them back afterwards by
applying the stabilizer to every best move:

```cpp
// After the loops, when the search was not stopped and stab is non-empty.
std::size_t searched = best_moves.size();
for (std::size_t k = 0; k < searched; ++k)
  for (const Transform &t : stab) {
    Move m = best_moves[k].move;
    Move image = {m.cell < 0 ? m.cell : t.cell[m.cell],
                  m.give < 0 ? m.give : t.piece[m.give]};
    bool known = std::any_of(best_moves.begin(), best_moves.end(),
                             [&](const Result &r) { return r.move == image; });
    if (!known)
      best_moves.push_back({best, image});
  }
```

At the very first move, add all 16 gives instead. The random pick then chooses
among the same moves as without symmetry, so a fixed seed still produces a
reproducible game.

### 5.5 Size of the savings

- **Empty board:** 16 gives become 1.
- **First placement:** 16 cells become 2, one on a diagonal and one off
  them. "Inside out" sends a corner to the centre, so corner and centre cells
  are equivalent here, unlike under rotations alone. The stabilizer at this
  position has 32 × 24 = 768 elements: every board symmetry, combined with
  every relabeling that fixes the piece in hand.
- **Later:** the stabilizer shrinks quickly and is usually only the identity
  after a few pieces. Then `c.symmetric` is false and generation costs
  nothing extra.

---

## 6. Settings for a full solve

### 6.1 Depth and position

Solving means searching until every line ends in a win or a full board:

```
position ..../..../..../....|-
go depth 16
```

The `|-` (nothing in hand) is the opening, where the root only gives a piece.
At depth 16 the search never cuts off at `eval`, so every score is exact.
Expected result: score 0, a draw.

Iterative deepening (1…16) costs little, because the shallow iterations fill
the TT with good moves for the deep one. Keep it.

### 6.2 Transposition table size and replacement

An entry is 16 bytes: the key's 64 bits, then the key's other 20 bits, score,
move, depth, bound and generation packed into a second word. Depth 0 marks an
empty entry, since every stored position has depth 1 or more. The slot is a
mix of the key scaled by `(mix * size) >> 64`, so the table can be any size,
not just a power of two. The engine's `DEFAULT_TT_SIZE` of 2048 MB holds
134 M entries; `optimal --solve` and `optimal --full` default to 8192 MB,
537 M entries. A full
solve visits far more positions than that, so give it as much memory as the
machine can spare.

The table has one entry per slot. A depth-preferred entry from the current
generation can only be replaced by an equal or deeper one, so new shallow
entries are dropped once the table fills up. A **two-entry bucket** is the
usual fix:

```cpp
struct Bucket {
  Entry deep;   // replaced only by deeper entries or an older generation
  Entry recent; // always replaced
};
```

- `probe`: check both entries for the key.
- `store`: if either entry has the key, update that entry. Otherwise, if
  `depth >= deep.depth` or `deep.gen != gen_`, write to `deep`. Otherwise,
  write to `recent`.

At 32 bytes per bucket, two buckets fit in a 64-byte cache line.

### 6.3 Move ordering

The number of nodes alpha-beta visits depends heavily on ordering. `Search`
orders cells and gives by history (`ordering.cpp`): about 10% fewer nodes, but
no faster, since picking the best move costs time at every node. The solver
keeps index order: history ordering made it 16-23% slower on 3- and 5-piece
solves, and only 10% faster on 4-piece ones. Most solved positions are draws,
and history says nothing about which move avoids losing. After the TT move,
cheap things to try:

- **Gives:** first try the piece that leaves the opponent the fewest
  placements that don't lose. `winning_pieces` already computes which pieces
  are deadly after each placement.
- **Placements:** first try placements that don't create a new line of three
  that could be completed. Such a line takes away safe gives for both sides.
- A **history table** indexed by `[cell][piece]` (moves that caused cutoffs
  before) is cheap and helps in Quarto as it does in chess.

None of this affects correctness, so it can be tuned independently after
symmetry works.

---

## 7. Testing

There is no test target yet. Add a small `odin_tests` executable that links
`odin_core`, with these checks:

**Symmetry tables**

- `SYMS[0]` is the identity, and all 32 rows are distinct permutations.
- Every `SYMS[s]` maps every bitboard in `LINES` to a bitboard in `LINES`.
- Closure: for every pair of symmetries, their composition is in `SYMS`.
- `INV_SYMS[s][SYMS[s][c]] == c`.
- For every s and all 65,536 bitboards, `apply_sym` matches a bit-by-bit
  reference implementation.

**Canonical form**

- **Invariance:** generate random legal positions (random move sequences from
  the empty board) and apply a random board symmetry plus a random relabeling
  (permutation and flip mask) to the board and the hand. The two keys must be
  equal. Run thousands of these.
- **Separation (sanity):** across many random positions, two different keys
  should be common. If almost every position maps to the same key, the
  normalisation is broken.
- **Known counts:** the empty board with nothing in hand gives one class. Its
  16 give-children give one class. Placing the held piece gives 2 classes of
  cells (diagonal and off-diagonal).
- **`symmetric`:** true for the empty board and for one piece on the board;
  false for most random positions with 6 or more pieces.

**Stabilizer**

- Every element maps the position onto itself: apply it to every piece on the
  board and to the hand, and compare with the original.
- The identity is always included, and there are no duplicates.
- Size checks: empty board with a piece in hand gives 768. For random
  positions, compare against a brute force over all 12,288 transformations.
- The skip masks keep exactly one move per group: for random symmetric
  positions, compare the generated moves against grouping all legal moves by
  their child's canonical key.

**Move mapping**

- `from_canonical(to_canonical(m, c), c) == m` for every cell and piece.
- The canonical move is legal in the canonical position: build the
  canonical position explicitly, check the mapped cell is empty and the mapped
  piece is available.

**Search**

- **Same results:** on a fixed set of positions (for example, 200 random
  positions with 4 to 10 pieces), `search(pos, d)` with symmetry on and off
  must return the same **score** for every depth d. Moves may differ when
  they're tied, but the score must not.
- **Fewer nodes:** with symmetry on, the node count should be lower. If it
  is higher, the TT move mapping is the first thing to check.
- **Solve curve:** time full solves from positions with 8, 6, 4 and 2 pieces
  before attempting the empty board, and use them to tune `CANON_MAX_PIECES`.
  A value that is too high makes deep nodes slow; one that is too low loses
  transpositions in the middle game.

---

## 8. Implementation order

1. Tables (`SYMS`, `INV_SYMS`, `SYM_BITS`, `apply_sym`) and their tests.
2. `canonicalize`, `to_canonical`, `from_canonical` and their tests.
3. Canonical keys in `negamax` (§4), and check "same results, fewer nodes".
4. `stabilizer` and its tests, then skipping at the root (§5.4), then in
   `negamax` (§5.2).
5. A two-entry TT bucket and a large TT (§6.2).
6. Move ordering (§6.3).
7. Solve curve from about 8 pieces down to the empty board.

Each step keeps the engine correct on its own, so you can benchmark after
every step and drop any step that doesn't pay off.

---

## 9. Perfect play afterwards

Koolen precomputed all 106,156 distinct positions in the first 10 plies, which
took about two weeks. That isn't necessary to play perfectly. Once the empty
board solves in reasonable time:

- Store the solved values of positions in the first few plies as an opening
  book, keyed by the canonical position (`cols`, `occ`). Book moves are stored
  in the canonical frame and mapped back with `from_canonical`.
- Solve every later position live with `go depth 16`.

Where the book ends depends on how fast the live solve is. Choose it after
timing the solve curve from §7.
