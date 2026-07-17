// Copyright 2026, Trail of Bits. All rights reserved.
//
// The StateCell store: standing per-group state for ONE aggregating functor
// (a QueryAggregate or a desugared QueryKVIndex). It is the runtime backing
// of the delta-relational-IR `StateCell` engine-scalar-family value
// (v3-spec-statecell.md §1, the R3 spec extension). This is the SINGLE new
// emitted primitive R3 adds; everything downstream of emit_touched's two
// UPDATECOUNTs rides existing DiffTable machinery.
//
// NON-ALIASING (spec §1.1, the compaction answer): StateCell dense group ids
// are a SEPARATE id space from the agg DiffTable's row ids. The DiffTable
// holds output rows (group.cols ++ summary-value) keyed by the WHOLE tuple;
// the StateCell holds standing state keyed by (group ++ config) ONLY (no
// summary value in the key). A DiffTable COMPACTION (Table.h CompactDead,
// which renumbers row ids) NEVER touches this store — no StateCell id is a
// DiffTable row id and no DiffTable index projects a StateCell id, so the
// cursor-contract renumbering leaves group ids stable. A group id, once
// allocated, is retained for the program's life even if the group empties
// (its working value returns to the algebra's identity): group ids are an
// append-only dense namespace, exactly the "monotone tables never compact"
// rationale. StateCell compaction (a separate renumbering with its own
// moved() callback, mirroring CompactRowsInPlace) is explicitly OUT of R3
// scope — a D5-style residue if a future COST measurement shows dead-group
// bloat.
//
// THE TWO-WORD CELL (spec §1.2, resolving C-0b/G-3): per dense group id the
// store holds a `working` blob (the current-epoch folded value, MUTATED by
// Fold, READ by Emit — a real read/write hazarded value) and a `sealed` blob
// (the batch-start snapshot, WRITTEN only by Seal, READ by Old — a frozen
// kInI-like value that is RAW-only against Seal and never a within-band
// hazard). old() is the FIRST VALUED sealed read: a Table's kInI answers
// "was this ROW present at batch start" with one id-compare; StateCell.Old
// answers "what was this GROUP's VALUE at batch start" with one sealed-blob
// load. Two words (not one + a history log) make new != old exact and O(1):
// a SUM cell folded -1 then +1 within a batch must still let Old report the
// pre-batch value while working already moved.
//
// OCCUPANCY (spec §C-1, the batch-1 abort fix): a group is EMPTY or OCCUPIED.
// The identity-initialized sealed value of a never-touched group is NOT a real
// batch-start row — a SUM of 0 is ambiguous between an empty group and a
// zero-sum group, so value-based suppression cannot distinguish group birth
// (empty -> occupied, emit +new only, no phantom -old) from group death
// (occupied -> empty, emit -old only, no phantom +new). The store therefore
// tracks a per-group WORKING member count (net signed, occupied <=> count > 0,
// algebra-independent — it counts folds, not values) and a SEALED occupancy bit
// (the batch-start snapshot, advanced by Seal alongside the sealed value).
// old() is defined only for a sealed-OCCUPIED group; WorkingOccupied()/
// SealedOccupied() drive the occupancy-generalized emit_touched guard (the
// generated code and the oracle read them; StateCell.h only maintains them):
//   birth  (empty->occupied):  emit ONLY +new;
//   death  (occupied->empty):  emit ONLY -old;
//   change (occupied, new!=old): the one net pair (-old, +new);
//   no-op  (occupancy unchanged and, if occupied, new==old): emit nothing
//          (covers OQ3 annihilation and the zero-sum-nonempty state).

#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <tuple>
#include <type_traits>
#include <utility>

#include "BenchCounters.h"
#include "Hash.h"
#include "Vec.h"

namespace hyde::rt {

// Sentinel dense group id meaning "no group".
inline constexpr uint32_t kNoGroup = ~0u;

// ---------------------------------------------------------------------------
// Algebra policies (spec §1.3 lowering fork; C-0e: a lowering SELECTOR, not
// an ordering fact). An Algebra is a compile-time policy — no vtables, repo
// idiom — supplying the three storage types and the fold/emit/old bodies.
// A policy exposes:
//   using Working;                 - the mutated per-group accumulator.
//   using Sealed;                  - the batch-start snapshot (an Emit result).
//   using Summary;                 - the emit() output columns.
//   static void Identity(Working&);          init working to the algebra id.
//   static void Fold(Working&, int32_t sign, Summary_&&... s);
//   static Summary Emit(const Working&);     working reduced to output cols.
//   static Sealed  SealFrom(const Working&); snapshot = Emit (the scalar).
//   static Summary OldOf(const Sealed&);     the frozen sealed read.
//   static constexpr bool kInvertible;       fold(-,v)∘fold(+,v) == id.
// ---------------------------------------------------------------------------

// (I) @invertible (abelian: fold+unfold O(1) both ways — COUNT/SUM/AVG). The
// working blob is a running reduction over a caller-supplied `Reduce` policy:
//   using Working / using Summary;  the two blob types.
//   static void Identity(Working&);
//   static void Combine(Working&, const Summary&);   working (+)= v
//   static void Uncombine(Working&, const Summary&); working (-)= v (unfold)
//   static Summary Finalize(const Working&);         e.g. AVG = Sum/Count
// For SUM/COUNT Working == Summary and Finalize is identity; for AVG the
// Working carries {sum,count} and Finalize divides. emit is O(1); the
// touched-group emit sweep is O(touched). This is what average_weight.dr's
// sum_i32 / count_i32 use.
// Detect a config-dependent emitted reduction policy: the generator stamps
// `static constexpr unsigned num_config` (and `kHasConfig`) plus a
// `ConfigTuple` (a std::tuple of the config column types) onto config-bearing
// `Reduce_<id>` policies (P2c). A config-FREE policy has neither, and every
// probe below compiles away.
template <typename Reduce>
concept HasConfigPolicy = requires { Reduce::kHasConfig; typename Reduce::ConfigTuple; };

// Fallback `ConfigTuple` provider for config-free policies (an empty tuple, so
// the DebugValidate std::apply over it expands to zero config probes).
struct IdentityConfigTuple {
  using ConfigTuple = std::tuple<>;
};

template <typename Reduce>
struct Invertible {
  using Working = typename Reduce::Working;
  using Summary = typename Reduce::Summary;
  using Sealed = Summary;  // The sealed snapshot is a prior Finalize() scalar.

  static constexpr bool kInvertible = true;

  // P2c: does the emitted reduction take leading config args? Config-free
  // policies leave this false and the DebugValidate config-probe machinery
  // compiles away (byte-identical to pre-P2c). `ConfigTuple` (a std::tuple of
  // the config column types) is exposed only for config-bearing cells so the
  // debug round-trip can synthesize value-init config probes.
  static constexpr bool kHasConfig = HasConfigPolicy<Reduce>;
  using ConfigTuple =
      typename std::conditional_t<HasConfigPolicy<Reduce>, Reduce,
                                  IdentityConfigTuple>::ConfigTuple;

  static void Identity(Working &w) {
    Reduce::Identity(w);
  }

  // Signed value-fold: +1 combines, -1 unfolds (the declared inverse). NO
  // presence crossing (spec C-0c): the crossing happens later, at
  // emit_touched, and only if new != old.
  //
  // The trailing pack is `[config...,] summary` (P2c). Forwarded RAW to the
  // fixed-signature `Reduce::Combine`/`Uncombine` the generator emits: config-
  // free = `(Working&, const Summary&)` (today, byte-identical), config-
  // dependent = `(Working&, cfg0.., const Summary&)`. A mid-signature config
  // pack is non-deducible in C++, so config MUST ride the trailing pack and
  // land on Combine's named params.
  template <typename... Args>
  static void Fold(Working &w, int32_t sign, Args &&...args) {
    if (0 < sign) {
      Reduce::Combine(w, static_cast<Args &&>(args)...);
    } else {
      Reduce::Uncombine(w, static_cast<Args &&>(args)...);
    }
  }

  // @invertible Emit/SealFrom are config-FREE (the config gate was applied
  // incrementally at Fold time; Finalize is identity/arithmetic on the running
  // reduction). The variadic `Cfg...` pack is accepted-and-ignored so the store
  // can call Emit/SealFrom uniformly across algebras; for @invertible it is
  // always empty (EmitGroupUpdate emits config at the Fold arm, not here).
  template <typename... Cfg>
  static Summary Emit(const Working &w, Cfg &&...) {
    return Reduce::Finalize(w);
  }

  template <typename... Cfg>
  static Sealed SealFrom(const Working &w, Cfg &&...) {
    return Reduce::Finalize(w);
  }

  static Summary OldOf(const Sealed &s) {
    return s;
  }
};

// (III) @recompute (opaque fallback — used for MIN/MAX (spec §6/G-5) and any
// functor whose invertibility is undeclared, e.g. average_weight.dr's
// new_weight_i32). The working blob holds the GROUP MEMBERSHIP (the raw
// summary values, with a signed multiplicity so retraction is exact);
// fold(+,v) appends / bumps, fold(-,v) decrements, emit() RE-RUNS the
// functor over the surviving members (the from-scratch per-group reduction).
// Cost: emit is O(group size), fired once per touched group per epoch —
// often the COST win for small groups. `Reduce` supplies only the
// from-scratch reduction over the live multiset:
//   using Summary;
//   static Summary ReduceLive(const Vec<Summary> &values,
//                             const Vec<int32_t> &counts);  rescan
//
// The multiset lives in parallel value/count Vecs. Those Vecs are NOT
// trivially copyable, so they cannot sit inside `Vec<Working>` by value
// (Vec<T> requires trivially-copyable T). The `Working` blob is therefore a
// POD handle: a pair of pointers to store-owned Vecs. The store allocates
// and destroys those Vecs through its Allocator (the `recompute_pool`), so
// this algebra keeps the "store is the sole allocation authority" contract.
// An `Invertible` Working is a plain value with no such pool, so the pool
// machinery compiles away for the abelian case.
template <typename Reduce>
struct Recompute {
  using Summary = typename Reduce::Summary;

  // A POD handle into the store-owned membership pool. Both pointers are null
  // until FindOrAddGroup wires them (spec §1.4 columnar-by-dense-group-id).
  // Entry i is (values[i], counts[i]); counts[i] == 0 means the value is
  // currently absent (retracted to identity) but its slot is retained.
  struct Working {
    Vec<Summary> *values{nullptr};
    Vec<int32_t> *counts{nullptr};
  };
  static_assert(std::is_trivially_copyable_v<Working>,
                "@recompute Working must be a POD handle (Vec<Working>)");

  using Sealed = Summary;  // The prior Emit() scalar, exactly as (I).

  static constexpr bool kInvertible = false;

  // P2c: does the emitted reduction take leading config args? Config-free
  // @recompute policies leave this false (byte-identical to pre-P2c); a
  // config-dependent @recompute (e.g. max_above) exposes it so the store's
  // group-birth guard (StateCell.h FindOrAddGroup) can value-init the sealed
  // placeholder instead of running a config-arity ReduceLive it has no config
  // for. Mirrors `Invertible::kHasConfig` (StateCell.h ~:130).
  static constexpr bool kHasConfig = HasConfigPolicy<Reduce>;

  // Wiring is done by the store (it owns the Vecs); nothing to init here.
  static void Identity(Working &) {}

  static void Fold(Working &w, int32_t sign, const Summary &v) {
    for (size_t i = 0u, n = w.values->Size(); i < n; ++i) {
      if ((*w.values)[i] == v) {
        w.counts->Set(i, (*w.counts)[i] + sign);
        return;
      }
    }
    w.values->Add(v);
    w.counts->Add(sign);
  }

  // Re-run the from-scratch reduction over the surviving (count > 0) members.
  // The leading `cfg...` pack is the config columns (P2c config-dependent
  // @recompute, e.g. max_above): config-free reductions pass no config and
  // `ReduceLive(values, counts)` binds byte-identically to today; config-
  // dependent reductions get `ReduceLive(cfg0.., values, counts)`. Config
  // enters @recompute HERE (the emit/reduce path), never at Fold — the Working
  // multiset stores summary values only.
  template <typename... Cfg>
  static Summary Emit(const Working &w, Cfg &&...cfg) {
    // Reduce sees the live multiset; @recompute functors are
    // order/multiplicity-defined by their own declared algebra (idempotent
    // MIN/MAX ignore multiplicity; a multiset SUM would honor it), so the
    // (values, counts) pair is faithful either way.
    return Reduce::ReduceLive(static_cast<Cfg &&>(cfg)..., *w.values,
                              *w.counts);
  }

  template <typename... Cfg>
  static Sealed SealFrom(const Working &w, Cfg &&...cfg) {
    return Emit(w, static_cast<Cfg &&>(cfg)...);
  }

  static Summary OldOf(const Sealed &s) {
    return s;
  }
};

// ---------------------------------------------------------------------------
// The store. Keyed (group ++ config) -> a dense group id; columnar
// working/sealed blobs indexed by that id (spec §1.4 class sketch).
// ---------------------------------------------------------------------------
template <typename Key, typename Algebra>
class StateCellStore {
 public:
  using Working = typename Algebra::Working;
  using Sealed = typename Algebra::Sealed;
  using Summary = typename Algebra::Summary;

  explicit StateCellStore(Allocator allocator_)
      : allocator(allocator_),
        keys(allocator_),
        hashes(allocator_),
        working(allocator_),
        working_count(allocator_),
        sealed(allocator_),
        sealed_occupied(allocator_),
        touched(allocator_),
        touched_flag(allocator_),
        mem_values_pool(allocator_),
        mem_counts_pool(allocator_) {}

  ~StateCellStore(void) {
    if (slots) {
      allocator.FreeArray(slots, slot_capacity);
    }
    // @recompute working handles point at store-owned membership Vecs; free
    // them here (for Invertible this compiles away — no pool entries exist).
    FreeMembership();
  }

  StateCellStore(const StateCellStore &) = delete;
  StateCellStore &operator=(const StateCellStore &) = delete;

  // Number of dense group ids ever allocated (append-only namespace).
  uint32_t NumGroups(void) const noexcept {
    return static_cast<uint32_t>(keys.Size());
  }

  // Dense group id for `key`, or `kNoGroup` (no allocation).
  uint32_t FindGroup(const Key &key) const noexcept {
    HYDE_RT_BENCH_COUNT(finds);
    if (!slot_capacity) {
      return kNoGroup;
    }
    const uint64_t hash = key.Hash();
    for (size_t i = hash & (slot_capacity - 1u);;
         i = (i + 1u) & (slot_capacity - 1u)) {
      HYDE_RT_BENCH_COUNT(probe_steps);
      const uint32_t gid = slots[i];
      if (gid == kNoGroup) {
        return kNoGroup;
      }
      if (hashes[gid] == hash && keys[gid] == key) {
        return gid;
      }
    }
  }

  // Dense group id for `key`, allocating (identity-init working+sealed) on
  // first touch. Mirrors RowStore::FindOrAdd (Table.h:152).
  uint32_t FindOrAddGroup(const Key &key) {
    const uint64_t hash = key.Hash();
    if (const uint32_t gid = FindGroupWithHash(key, hash); gid != kNoGroup) {
      return gid;
    }
    const uint32_t gid = NumGroups();
    if (gid == kNoGroup) {
      std::fprintf(stderr, "hyde::rt: state-cell group-id space exhausted\n");
      std::abort();
    }
    keys.Add(key);
    hashes.Add(hash);
    Working w{};
    Algebra::Identity(w);
    InitMembership(w);
    working.Add(w);
    working_count.Add(0);  // Fresh group is EMPTY (no members folded yet).
    Sealed s{};
    // The batch-start snapshot of a never-touched group is the algebra
    // identity's Emit — the group's summary before any fold ever ran. It is a
    // placeholder; old() is only meaningful once the group is sealed-OCCUPIED,
    // which a never-touched group is not.
    //
    // P2c config-@recompute (A-F1): a config-dependent @recompute cell's
    // SealFrom(w) -> Emit(w) -> ReduceLive(cfg0.., values, counts) is arity-3
    // and FindOrAddGroup has no per-group config, so SealFrom(w) (no config)
    // would arity-mismatch at COMPILE time. The birth value is INERT
    // (sealed_occupied=0 makes old() undefined until a real Seal overwrites it),
    // so a value-init `Sealed{}` placeholder is sound and needs no config. The
    // guard confines `Sealed{}` to exactly config-@recompute cells; every other
    // class keeps `SealFrom(w)` BYTE-IDENTICAL (@invertible/config-free:
    // Finalize(identity) == 0 == Sealed{}; config-free @recompute: unchanged).
    if constexpr (Algebra::kInvertible) {
      sealed.Add(Algebra::SealFrom(w));
    } else if constexpr (Algebra::kHasConfig) {
      sealed.Add(s);  // inert value-init placeholder (never read before a Seal)
    } else {
      sealed.Add(Algebra::SealFrom(w));
    }
    sealed_occupied.Add(0u);  // Never occupied at batch start.
    (void) s;
    touched_flag.Add(0u);
    InsertSlot(gid, hash);
    return gid;
  }

  // VALUE-fold, NO presence crossing (spec C-0c). Records the group in
  // `touched` exactly once (mirror DiffTable::Touch, Table.h:657).
  //
  // The trailing pack is `[config...,] summary` (P2c config-column aggregates):
  // for a config-FREE aggregate it is a single summary scalar and binds
  // `Algebra::Fold`'s `const Summary &v` byte-identically to the pre-P2c
  // `Summary(v)` wrapper (which existed only to normalize a brace-init pack and
  // was a no-op for a scalar summary). For a config-DEPENDENT @invertible
  // aggregate the caller passes `(cfg0.., v)` and the pack is forwarded RAW to
  // the fixed-signature `Reduce::Combine(Working&, cfg0.., const Summary&)` the
  // generator emits — no `Summary(...)` reconstruction (which would collapse
  // config into the summary type / arity-mismatch Combine). @recompute cells
  // never receive config here (config routes through Emit/ReduceLive instead).
  template <typename... Args>
  void Fold(uint32_t gid, int32_t sign, Args &&...args) {
    HYDE_RT_BENCH_COUNT(folds_plus);  // Sign narrated by the caller's arms.
    Touch(gid);
    // Vec exposes only a const operator[] and Set (compaction must never hand
    // out a mutable reference into the backing array). Read the blob, fold it,
    // write it back. For @invertible this rewrites the scalar; for @recompute
    // the handle is unchanged (the pointed-to membership Vecs mutate in place)
    // and the write-back is a harmless POD store.
    Working w = working[gid];
    Algebra::Fold(w, sign, static_cast<Args &&>(args)...);
    working.Set(gid, w);
    // Occupancy (spec §C-1): each fold is one signed member; the net count
    // drives WorkingOccupied. A COUNT algebra's working value happens to equal
    // this count, but SUM/MIN do not, so occupancy is tracked separately and
    // algebra-independently. May dip below zero mid-epoch (a retraction seen
    // before its addition), exactly like the split derivation counters.
    working_count.Set(gid, working_count[gid] + sign);
  }

  // Occupancy queries (spec §C-1) — the occupancy-generalized emit_touched
  // guard reads these. WorkingOccupied: does the group have live members THIS
  // epoch (net fold count > 0). SealedOccupied: was the group occupied at batch
  // start (old() is defined only when this is true).
  bool WorkingOccupied(uint32_t gid) const noexcept {
    return 0 < working_count[gid];
  }
  bool SealedOccupied(uint32_t gid) const noexcept {
    return 0u != sealed_occupied[gid];
  }

  // Current working value reduced to output columns. emit(g) — a VALUED read
  // of the working word, RAW after every Fold this epoch. The optional leading
  // `cfg...` pack (P2c config-dependent @recompute) is forwarded to the
  // reduction; @invertible ignores it. Config-free call sites pass no config
  // and this binds byte-identically to today's `Emit(gid)`.
  template <typename... Cfg>
  Summary Emit(uint32_t gid, Cfg &&...cfg) const {
    HYDE_RT_BENCH_COUNT(member_checks);
    return Algebra::Emit(working[gid], static_cast<Cfg &&>(cfg)...);
  }

  // Batch-start snapshot (frozen; the kInI-analogue, VALUED). old(g).
  Summary Old(uint32_t gid) const {
    HYDE_RT_BENCH_COUNT(member_checks);
    return Algebra::OldOf(sealed[gid]);
  }

  // End-of-epoch: for each touched group, sealed := Emit(working); clear
  // touched. Mirror of Table::Seal (Table.h:277) / DiffTable Commit's kInI
  // advance. Emitted at the commit-sweep tail (STATE_SEAL), AFTER
  // emit_touched has fired all pairs THIS epoch (spec §2.4 E5).
  void Seal(void) {
    HYDE_RT_BENCH_COUNT_N(commit_visits, touched.Size());
    for (uint32_t gid : touched) {
      sealed.Set(gid, Algebra::SealFrom(working[gid]));
      // Advance the batch-start occupancy snapshot alongside the value (spec
      // §C-1): next epoch's SealedOccupied/Old reflect THIS epoch's final state.
      sealed_occupied.Set(gid, (0 < working_count[gid]) ? 1u : 0u);
      touched_flag.Set(gid, 0u);
    }
    touched.Clear();
  }

  // P2c config-@recompute (fork (i), DemandSeeds §2.1): one group's seal with a
  // caller-supplied config pack. IS the per-group bookkeeping body of Seal()
  // above (sealed/sealed_occupied/touched_flag writes), factored so a codegen
  // per-touched-group loop can pass KeyAt(gid)'s config slots to
  // SealFrom -> ReduceLive (a config-dependent @recompute cell needs the
  // group's config, which the bulk Seal() cannot conjure). Config-free /
  // @invertible cells never reach here — they keep the opaque bulk Seal().
  // The bench counter mirrors Seal()'s discipline: one visit per gid (A-F2).
  template <typename... Cfg>
  void SealOne(uint32_t gid, Cfg &&...cfg) {
    HYDE_RT_BENCH_COUNT(commit_visits);
    sealed.Set(gid, Algebra::SealFrom(working[gid], static_cast<Cfg &&>(cfg)...));
    sealed_occupied.Set(gid, (0 < working_count[gid]) ? 1u : 0u);
    touched_flag.Set(gid, 0u);
  }

  // Clear the touched set after a codegen-driven per-group SealOne loop (fork
  // (i)): the bulk Seal() clears internally, but the codegen loop needs an
  // explicit end. Mirrors the tail of Seal().
  void ClearTouched(void) { touched.Clear(); }

  // Iteration for emit_touched: the SORT-UNIQUE touched set (spec §2.5
  // V-TOUCH-SORTED / G-8). Touch appends each gid at most once, so the
  // underlying vector is already unique; Sort makes the emit order canonical.
  //
  // Returns a CONST reference (review finding 1): the caller must never be
  // handed a mutable ref to `touched`, because appending/clearing through it
  // would DESYNC `touched` from `touched_flag` (the two are a paired dedup
  // structure — Touch keeps them in lockstep, Seal clears both). The in-place
  // SortAndUnique only REORDERS the same gids (Seal iterates order-
  // independently), so it preserves the pairing; only external mutation could
  // break it, and the const result forbids that. This is NOT declared `const`
  // (SortAndUnique mutates the member) but the RESULT is const per the spec.
  const Vec<uint32_t> &Touched(void) {
    touched.SortAndUnique();
    return touched;
  }

  const Key &KeyAt(uint32_t gid) const noexcept {
    return keys[gid];
  }

  // §1.5 declared-algebra property checks + seal coherence + non-aliasing.
  void DebugValidate(void) const;

 private:
  uint32_t FindGroupWithHash(const Key &key, uint64_t hash) const noexcept {
    HYDE_RT_BENCH_COUNT(finds);
    if (!slot_capacity) {
      return kNoGroup;
    }
    for (size_t i = hash & (slot_capacity - 1u);;
         i = (i + 1u) & (slot_capacity - 1u)) {
      HYDE_RT_BENCH_COUNT(probe_steps);
      const uint32_t gid = slots[i];
      if (gid == kNoGroup) {
        return kNoGroup;
      }
      if (hashes[gid] == hash && keys[gid] == key) {
        return gid;
      }
    }
  }

  void InsertSlot(uint32_t gid, uint64_t hash) {
    const size_t num_groups = keys.Size();
    if ((num_groups + (num_groups >> 3u)) >= slot_capacity) {
      Rehash();
      return;
    }
    for (size_t i = hash & (slot_capacity - 1u);;
         i = (i + 1u) & (slot_capacity - 1u)) {
      if (slots[i] == kNoGroup) {
        slots[i] = gid;
        return;
      }
    }
  }

  void Rehash(void) {
    HYDE_RT_BENCH_COUNT(rehash_events);
    HYDE_RT_BENCH_COUNT_N(rehash_rows, NumGroups());
    const size_t new_capacity = slot_capacity ? slot_capacity * 2u : 64u;
    auto new_slots = allocator.AllocateArray<uint32_t>(new_capacity);
    for (size_t i = 0u; i < new_capacity; ++i) {
      new_slots[i] = kNoGroup;
    }
    if (slots) {
      allocator.FreeArray(slots, slot_capacity);
    }
    slots = new_slots;
    slot_capacity = new_capacity;
    for (uint32_t gid = 0u; gid < NumGroups(); ++gid) {
      const uint64_t hash = hashes[gid];
      for (size_t i = hash & (slot_capacity - 1u);;
           i = (i + 1u) & (slot_capacity - 1u)) {
        if (slots[i] == kNoGroup) {
          slots[i] = gid;
          break;
        }
      }
    }
  }

  // Record `gid` in the touched set exactly once per epoch. Mirror of
  // DiffTable::Touch's kTouched discipline (Table.h:657): a per-group dedup
  // byte, not a scan.
  void Touch(uint32_t gid) {
    HYDE_RT_BENCH_COUNT(touch_calls);
    if (!touched_flag[gid]) {
      HYDE_RT_BENCH_COUNT(touch_appends);
      touched_flag.Set(gid, 1u);
      touched.Add(gid);
    }
  }

  // --- @recompute membership pool wiring (no-op for Invertible) ------------
  // The @recompute `Working` is a POD handle {Vec<Summary>*, Vec<int32_t>*}
  // into per-group value/count Vecs the STORE owns (so the store stays the
  // sole allocation authority — every allocation flows through `allocator`).
  // The inner Vec types are known at compile time, so no type erasure is
  // needed: two pointer-pools hold the heap Vecs and destroy them by their
  // known type at teardown. For `Invertible`, `Working` is a plain value with
  // no `.values`/`.counts` members, and every branch below compiles away.
  static constexpr bool kHasMembership =
      requires(Working w) { w.values; w.counts; };

  // The element types of the two membership Vecs (only referenced when
  // kHasMembership; guarded so the Invertible instantiation stays clean).
  template <bool HasMem, typename W>
  struct MemTypes {
    using Values = int;  // Unused placeholder for the Invertible case.
    using Counts = int;
  };
  template <typename W>
  struct MemTypes<true, W> {
    using Values = std::remove_pointer_t<decltype(std::declval<W>().values)>;
    using Counts = std::remove_pointer_t<decltype(std::declval<W>().counts)>;
  };
  using ValuesVec = typename MemTypes<kHasMembership, Working>::Values;
  using CountsVec = typename MemTypes<kHasMembership, Working>::Counts;

  // Wire a fresh @recompute group's Working handle to two store-owned Vecs.
  void InitMembership(Working &w) {
    if constexpr (kHasMembership) {
      w.values = MakeVec<ValuesVec>(mem_values_pool);
      w.counts = MakeVec<CountsVec>(mem_counts_pool);
    } else {
      (void) w;
    }
  }

  template <typename VecT>
  VecT *MakeVec(Vec<VecT *> &pool) {
    void *mem = allocator.Allocate(sizeof(VecT), alignof(VecT));
    auto *p = new (mem) VecT(allocator);
    pool.Add(p);
    return p;
  }

  void FreeMembership(void) {
    if constexpr (kHasMembership) {
      for (ValuesVec *p : mem_values_pool) {
        p->~ValuesVec();
        allocator.Free(p, sizeof(ValuesVec), alignof(ValuesVec));
      }
      for (CountsVec *p : mem_counts_pool) {
        p->~CountsVec();
        allocator.Free(p, sizeof(CountsVec), alignof(CountsVec));
      }
    }
  }

  Allocator allocator;
  Vec<Key> keys;          // Dense group id -> key (KeyAt + debug).
  Vec<uint64_t> hashes;   // Parallel to keys; cached key hashes.
  Vec<Working> working;   // Columnar by dense group id (the mutated value).
  Vec<int32_t> working_count;  // Net signed member count this epoch (spec §C-1;
                               // occupied <=> > 0; algebra-independent).
  Vec<Sealed> sealed;     // Batch-start snapshot (old()).
  Vec<uint8_t> sealed_occupied;  // Batch-start occupancy bit (spec §C-1).
  Vec<uint32_t> touched;  // Groups folded this epoch.
  Vec<uint8_t> touched_flag;      // Per-group append-once bit (kTouched mirror).
  Vec<ValuesVec *> mem_values_pool;  // Owned @recompute value Vecs (else empty).
  Vec<CountsVec *> mem_counts_pool;  // Owned @recompute count Vecs (else empty).
  uint32_t *slots{nullptr};    // Open-addressing key -> gid (mirror RowStore).
  size_t slot_capacity{0u};
};

template <typename Key, typename Algebra>
void StateCellStore<Key, Algebra>::DebugValidate(void) const {
#ifndef NDEBUG
  // SEAL COHERENCE (V-CELL-SEAL, spec §1.5/§2.5): after Seal(), touched is
  // empty and no touched-flag survives. Structural guard on epoch order (no
  // Fold-after-Seal leak within the epoch).
  assert(touched.Empty());
  for (uint32_t gid = 0u; gid < NumGroups(); ++gid) {
    assert(0u == touched_flag[gid]);
    // OCCUPANCY COHERENCE (spec §C-1): after Seal the sealed occupancy bit
    // agrees with the (now stable) working member count, and the count is never
    // negative at a batch boundary (it may only dip below zero mid-epoch).
    assert(0 <= working_count[gid]);
    assert((0u != sealed_occupied[gid]) == (0 < working_count[gid]));
  }

  // ALGEBRA LAWS (spec §1.5; AggregatingFunctors §3 "annotations can lie").
  // For @invertible, Uncombine inverts Combine bit-for-bit: fold(+,v) then
  // fold(-,v) restores working exactly (unfold is a true inverse). Sample a
  // bounded number of groups per call (cost-bounded, like
  // DebugValidateCounts' per-row scan). Only checkable when the algebra
  // exposes a re-foldable working (Invertible); @recompute working is a
  // pointer bundle and is validated by the oracle's from-scratch equality
  // (spec §4) instead.
  if constexpr (Algebra::kInvertible) {
    constexpr uint32_t kSampleCap = 64u;
    const uint32_t n = NumGroups();
    const uint32_t step = n > kSampleCap ? n / kSampleCap : 1u;
    for (uint32_t gid = 0u; gid < n; gid += step) {
      // A round-trip on a probe value must be identity on the emitted summary.
      Working w = working[gid];
      const Summary before = Algebra::Emit(w);
      Summary probe{};
      if constexpr (Algebra::kHasConfig) {
        // P2c option (i): a config-DEPENDENT reduction's Combine/Uncombine take
        // leading config args. Synthesize identity (value-initialized) config
        // probes — the invertibility law Uncombine(cfg, Combine(cfg,w,v), v)==w
        // holds for ANY FIXED cfg (fold and unfold apply the same cfg-gate), so
        // a value-init cfg is a sound representative and keeps the round-trip
        // check alive for config cells. `ConfigTuple` value-inits all cfg
        // scalars; std::apply prepends them ahead of the summary probe.
        typename Algebra::ConfigTuple cfg{};
        std::apply(
            [&](auto &...c) { Algebra::Fold(w, +1, c..., probe); }, cfg);
        std::apply(
            [&](auto &...c) { Algebra::Fold(w, -1, c..., probe); }, cfg);
      } else {
        Algebra::Fold(w, +1, probe);
        Algebra::Fold(w, -1, probe);
      }
      const Summary after = Algebra::Emit(w);
      assert(before == after);  // unfold ∘ fold == id on the summary.
    }
  }
#endif
}

}  // namespace hyde::rt
