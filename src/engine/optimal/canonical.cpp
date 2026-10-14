
#include "canonical.hpp"
#include "tables.hpp"

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
  const std::uint32_t allowed = squares_win ? SQUARE_SYMS : ~0u;
  occ = pos.occupied();
  std::uint32_t candidates = 1; // bit s set: SYMS[s] reaches occ
  for (int s = 1; s < SYM_COUNT; ++s) {
    if (!((allowed >> s) & 1))
      continue;
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
// is the occupancy under s.
//
// Inverting property i turns its column p into p ^ occ. With a piece in hand
// the inversions are chosen to turn it into piece 0; with none, each column
// takes whichever of p and p ^ occ is smaller. Either way the choice no longer
// depends on how the properties were inverted. Reordering properties reorders
// the columns, so sorting them removes the order as well.
Canon normalise(const Position &pos, int s, Bitboard occ) {
  const Piece hand = pos.in_hand();
  Canon n{0, occ, hand >= 0, std::uint8_t(s), {0, 1, 2, 3}, 0};
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

} // namespace

Canon canonicalize(const Position &pos) {
  Bitboard occ;
  std::uint32_t candidates = occ_candidates(pos, occ);

  // Every candidate has the same occupancy, so only the columns decide.
  // Starts from the first candidate, like occ_candidates: no cols value is
  // free to serve as "none yet".
  Canon best = normalise(pos, std::countr_zero(candidates), occ);
  for (candidates &= candidates - 1; candidates; candidates &= candidates - 1) {
    Canon n = normalise(pos, std::countr_zero(candidates), occ);
    if (n.cols < best.cols)
      best = n;
  }
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

} // namespace quarto
