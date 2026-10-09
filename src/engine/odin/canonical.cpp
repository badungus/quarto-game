
#include "canonical.hpp"
#include "tables.hpp"

#include <algorithm>
#include <bit>

namespace quarto {

namespace {
// SYM_BITS[s].lo[b] is the low byte b with every bit moved by SYMS[s]; .hi the
// same for the high byte.
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

constexpr auto SYM_BITS = make_sym_bits();

constexpr Bitboard apply_sym(int s, Bitboard b) {
  return Bitboard(SYM_BITS[s].lo[b & 0xFF] | SYM_BITS[s].hi[b >> 8]);
}

// Sorts q ascending and carries the original property index along.
void sort4(std::array<Bitboard, 4> &q, std::array<std::uint8_t, 4> &idx) {
  auto cmp_swap = [&](int i, int j) {
    if (q[j] < q[i]) {
      std::swap(q[i], q[j]);
      std::swap(idx[i], idx[j]);
    }
  };
  // Optimal 5-comparator network for 4 elements.
  cmp_swap(0, 1);
  cmp_swap(2, 3);
  cmp_swap(0, 2);
  cmp_swap(1, 3);
  cmp_swap(1, 2);
}

// The symmetries with the smallest permuted occupancy, as a bit mask. The
// canonical form has the smallest occupancy first, so only these can reach
// it. Sets occ to that occupancy. Starts from the identity, SYMS[0]: a full
// board is 0xFFFF, so no Bitboard value is free to serve as "none yet".
std::uint32_t occ_candidates(const Position &pos, Bitboard &occ) {
  occ = pos.occupied();
  std::uint32_t candidates = 1; // bit s set: SYMS[s] reaches occ
  for (int s = 1; s < SYM_COUNT; ++s) {
    Bitboard o = apply_sym(s, pos.occupied());
    if (o < occ) {
      occ = o;
      candidates = 1u << s;
    } else if (o == occ) {
      candidates |= 1u << s;
    }
  }
  return candidates;
}

// The position under board symmetry s with the relabeling normalised. `occ`
// is the occupancy under s. `symmetric` is left false for canonicalize.
//
// Inverting property i turns its column p into p ^ occ. With a piece in hand
// the inversions are chosen to turn it into piece 0; with none, each column
// takes whichever of p and p ^ occ is smaller. Either way the choice no longer
// depends on how the properties were inverted. Reordering properties reorders
// the columns, so sorting them removes the order as well.
Canon normalise(const Position &pos, int s, Bitboard occ) {
  const Piece hand = pos.in_hand();
  Canon n{0, occ, hand >= 0, std::uint8_t(s), {0, 1, 2, 3}, 0, false};
  std::array<Bitboard, 4> q;
  for (int i = 0; i < 4; ++i) {
    Bitboard p = apply_sym(s, pos.props()[i]);
    Bitboard inv = Bitboard(p ^ occ);
    bool invert = hand >= 0 ? ((hand >> i) & 1) != 0 : inv < p;
    if (invert) {
      p = inv;
      n.flip |= 1 << i;
    }
    q[i] = p;
  }
  sort4(q, n.order);
  for (int j = 0; j < 4; ++j)
    n.cols |= std::uint64_t(q[j]) << (16 * j);
  return n;
}

// Canonical bit j of a piece is original bit order[j], inverted by flip.
Piece relabel(Piece p, const std::array<std::uint8_t, 4> &order,
              std::uint8_t flip) {
  Piece r = 0;
  for (int j = 0; j < 4; ++j) {
    int i = order[j];
    if (((p >> i) ^ (flip >> i)) & 1)
      r |= 1 << j;
  }
  return r;
}

Piece unrelabel(Piece r, const std::array<std::uint8_t, 4> &order,
                std::uint8_t flip) {
  Piece p = 0;
  for (int j = 0; j < 4; ++j) {
    int i = order[j];
    if (((r >> j) ^ (flip >> i)) & 1)
      p |= 1 << i;
  }
  return p;
}

Transform make_transform(int s, const std::array<std::uint8_t, 4> &order,
                         std::uint8_t flip) {
  Transform t;
  t.cell = SYMS[s];
  for (int p = 0; p < 16; ++p)
    t.piece[p] = relabel(Piece(p), order, flip);
  return t;
}

Transform inverse(const Transform &t) {
  Transform inv;
  for (int x = 0; x < 16; ++x) {
    inv.cell[t.cell[x]] = Cell(x);
    inv.piece[t.piece[x]] = Piece(x);
  }
  return inv;
}

// a after b.
Transform compose(const Transform &a, const Transform &b) {
  Transform r;
  for (int x = 0; x < 16; ++x) {
    r.cell[x] = a.cell[b.cell[x]];
    r.piece[x] = a.piece[b.piece[x]];
  }
  return r;
}
} // namespace

Key Canon::key() const {
  // The occupancy and has_hand are part of the key: the same columns on
  // another board, or with and without a piece in hand, are different
  // positions. The top bit is forced so the key is never 0, which the TT
  // reserves for empty slots. Not bit 0: the TT indexes with the low bits, so
  // that would leave every even slot unused.
  Key state = cols;
  state = splitmix64(state) ^ (occ | (has_hand ? std::uint64_t{1} << 16 : 0));
  return splitmix64(state) | (Key{1} << 63);
}

Canon canonicalize(const Position &pos) {
  Bitboard occ;
  std::uint32_t candidates = occ_candidates(pos, occ);

  // Every candidate has the same occupancy, so only the columns decide.
  // Starts from the first candidate, like occ_candidates: no cols value is
  // free to serve as "none yet".
  Canon best = normalise(pos, std::countr_zero(candidates), occ);
  int ties = 1; // candidates reaching best.cols
  for (candidates &= candidates - 1; candidates; candidates &= candidates - 1) {
    Canon n = normalise(pos, std::countr_zero(candidates), occ);
    if (n.cols < best.cols) {
      best = n;
      ties = 1;
    } else if (n.cols == best.cols) {
      ++ties;
    }
  }

  // Two symmetries reaching the same columns, or two equal columns, each mean
  // a non-identity transformation fixes the position. The empty board with
  // nothing in hand is fixed by every relabeling.
  bool col_tie = best.col(0) == best.col(1) || best.col(1) == best.col(2) ||
                 best.col(2) == best.col(3);
  best.symmetric = ties > 1 || col_tie || (occ == 0 && !best.has_hand);
  return best;
}

Move to_canonical(Move m, const Canon &c) {
  Move r = EMPTY_MOVE;
  if (m.cell >= 0)
    r.cell = SYMS[c.sym][m.cell];
  if (m.give >= 0)
    r.give = relabel(m.give, c.order, c.flip);
  return r;
}

Move from_canonical(Move m, const Canon &c) {
  Move r = EMPTY_MOVE;
  if (m.cell >= 0)
    r.cell = INV_SYMS[c.sym][m.cell];
  if (m.give >= 0)
    r.give = unrelabel(m.give, c.order, c.flip);
  return r;
}

std::vector<Transform> stabilizer(const Position &pos, const Canon &c) {
  // T0 = the transformation canonicalize picked. Every T that also reaches
  // the canonical form gives T0^-1 after T, which maps pos onto itself.
  const Transform back = inverse(make_transform(c.sym, c.order, c.flip));
  Bitboard occ;
  std::uint32_t candidates = occ_candidates(pos, occ);
  std::vector<Transform> stab;

  for (; candidates; candidates &= candidates - 1) {
    int s = std::countr_zero(candidates);
    Canon n = normalise(pos, s, occ);
    // Every candidate has c's occupancy and hand, so equal columns mean
    // exactly the same canonical form.
    if (n.cols != c.cols)
      continue;
    // Equal columns can trade places: every reordering that leaves the
    // columns as they are reaches the same canonical form.
    std::array<std::uint8_t, 4> perm = {0, 1, 2, 3};
    do {
      bool keeps_cols = true;
      for (int j = 0; j < 4; ++j)
        keeps_cols = keeps_cols && n.col(perm[j]) == n.col(j);
      if (!keeps_cols)
        continue;
      std::array<std::uint8_t, 4> order;
      for (int j = 0; j < 4; ++j)
        order[j] = n.order[perm[j]];
      stab.push_back(compose(back, make_transform(s, order, n.flip)));
    } while (std::next_permutation(perm.begin(), perm.end()));
  }
  return stab;
}

} // namespace quarto
