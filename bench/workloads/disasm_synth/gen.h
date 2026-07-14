// Copyright 2026, Trail of Bits. All rights reserved.
//
// Knobbed, seeded synthetic disassembly-stream generator for the
// disasm_synth family (design R19: the realistic-program point). Mirrors
// the shape a recursive disassembler actually sees, scaled by knobs, so
// the engine driver ingests a program that structurally resembles machine
// code rather than a random relation.
//
// PROVENANCE: the datalog program under test is a verbatim copy of
// tests/MiniDisassembler/database.dr (see disasm.dr in this directory);
// that ctest case is the correctness net. The two messages are plain
// #message (instruction/1, raw_transfer/3) -- MONOTONE, add-only. There
// is NO @differential surface here, so this family is GROWTH/INGEST ONLY:
// there is no retraction, no edit_batches knob. The comment in disasm.dr
// and BASELINE.md both record the scope.
//
// Stream shape (all EAs are contiguous, as in a real image):
//
//   * `blocks` basic blocks are laid out back-to-back in EA space from a
//     base EA. Block k occupies a contiguous run of instruction EAs; the
//     next block begins one EA past the last instruction of block k (plus
//     a small inter-block gap so orphan bytes have somewhere to sit).
//   * Each multi-instruction block is a FALL_THROUGH chain: instruction at
//     EA_i, then raw_transfer(EA_i, EA_{i+1}, FALL_THROUGH) for consecutive
//     EAs. The block HEAD is the first EA of the chain.
//   * Block lengths are geometric with mean `block_len` (default 8),
//     clamped to [1, 4*block_len] so a runaway sample cannot dominate.
//   * A controlled `call_density` percent of blocks additionally emit a
//     CALL transfer from a random earlier instruction to THIS block's head
//     (targets are block heads only -- the realistic call-graph shape). A
//     block that is a call target and has an intra-block fall-through
//     predecessor still becomes a function head by the raw_transfer(_,H,_)
//     rule; a block whose head has NO fall-through predecessor is a
//     function head by the "no predecessor" rule. Both are exercised.
//   * `orphan_pct` percent of blocks are forced to length 1 (a lone
//     instruction with no fall-through in or out) -- the orphan-byte
//     fraction. Orphans are always function heads (no predecessor).
//
// The generator produces a flat, deterministic list of instruction EAs and
// typed transfers up front; the driver slices them into ingest batches of a
// knobbed size and times each message call. All three of {instructions,
// fall-through transfers, call transfers} are produced in EA order so the
// batch boundaries are reproducible from the knob string alone.

#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "../../common/bench.h"

namespace bench {

// A typed transfer edge in the synthetic stream. `kind` is 0 for
// FALL_THROUGH, 1 for CALL -- matching the #constant EdgeType values in
// disasm.dr. The driver maps these to the generated EdgeType enum without
// this header needing to include the generated code.
struct Transfer {
  uint64_t from;
  uint64_t to;
  uint8_t kind;  // 0 = FALL_THROUGH, 1 = CALL
};

struct DisasmStream {
  std::vector<uint64_t> instructions;  // all instruction EAs, ascending
  std::vector<Transfer> transfers;     // fall-through + call, EA-from order
  std::vector<uint64_t> block_heads;   // head EA of every block, ascending
};

// Build the whole synthetic program deterministically from the knobs.
//
//   blocks       number of basic blocks
//   block_len    mean fall-through chain length (geometric)
//   call_density percent of blocks that receive an inbound CALL
//   orphan_pct   percent of blocks forced to length 1 (orphan bytes)
//   base_ea      first instruction EA (default 0x1000; keeps EAs nonzero)
//   seed         PRNG seed
//
// Determinism: the only randomness is (a) each block's length, (b) whether
// a block is an orphan, (c) whether a block gets a call and from where.
// All draws come from one splitmix64 stream seeded by `seed`, in a fixed
// order (block by block), so the stream is a pure function of the knobs.
inline DisasmStream GenDisasm(uint32_t blocks, uint32_t block_len,
                              uint32_t call_density, uint32_t orphan_pct,
                              uint64_t base_ea, uint64_t seed) {
  DisasmStream s;
  Rng rng(seed * 0x9e3779b97f4a7c15ull + 0x1234u);

  const uint32_t max_len = block_len ? block_len * 4u : 4u;
  uint64_t ea = base_ea;

  // Pass 1: lay out blocks (instructions + intra-block fall-through), record
  // heads. We defer call transfers to pass 2 so a call can target any block
  // head (including later ones) without a forward-reference dance.
  std::vector<std::pair<uint64_t, uint64_t>> block_span;  // [head, last]
  block_span.reserve(blocks);
  for (uint32_t b = 0u; b < blocks; ++b) {
    const bool orphan = (orphan_pct != 0u) && (rng.Below(100u) < orphan_pct);

    uint32_t len;
    if (orphan) {
      len = 1u;
    } else if (block_len <= 1u) {
      len = 1u;
    } else {
      // Geometric-ish length with mean ~block_len: keep sampling a
      // continue-bit with probability (block_len-1)/block_len.
      len = 1u;
      while (len < max_len && rng.Below(block_len) != 0u) {
        len += 1u;
      }
    }

    const uint64_t head = ea;
    s.block_heads.push_back(head);
    uint64_t prev = 0u;
    bool have_prev = false;
    for (uint32_t i = 0u; i < len; ++i) {
      s.instructions.push_back(ea);
      if (have_prev) {
        s.transfers.push_back({prev, ea, 0u});  // FALL_THROUGH
      }
      prev = ea;
      have_prev = true;
      ea += 1u;
    }
    block_span.push_back({head, prev});
    ea += 1u;  // one-EA inter-block gap (keeps EAs distinct across blocks)
  }

  // Pass 2: call transfers. For each block chosen by call_density, emit a
  // CALL from a random already-emitted instruction to this block's head.
  // Source is any instruction with a strictly smaller EA than the target
  // head, so the (FromEA, ToEA, CALL) rule sees an instruction on both
  // sides (both the source instruction and the target head are emitted).
  for (uint32_t b = 0u; b < blocks; ++b) {
    if (call_density == 0u || rng.Below(100u) >= call_density) {
      continue;
    }
    const uint64_t target = block_span[b].first;
    // Find how many instructions precede `target` (they are ascending).
    // The instructions vector is sorted; the count of EAs < target is the
    // number of legal call sources. Binary search keeps generation cheap.
    size_t lo = 0u, hi = s.instructions.size();
    while (lo < hi) {
      const size_t mid = lo + (hi - lo) / 2u;
      if (s.instructions[mid] < target) {
        lo = mid + 1u;
      } else {
        hi = mid;
      }
    }
    if (lo == 0u) {
      continue;  // no earlier instruction to call from (first block)
    }
    const uint64_t src = s.instructions[rng.Below(lo)];
    s.transfers.push_back({src, target, 1u});  // CALL
  }

  return s;
}

}  // namespace bench
