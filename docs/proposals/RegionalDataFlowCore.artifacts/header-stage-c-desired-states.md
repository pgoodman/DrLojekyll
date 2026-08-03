# Header surface after Stage C — desired states as diffs

Charge: the generated `datalog.h` (the whole driver-facing surface) AFTER the
Stage-C cutover (`stage-c-diff.md`, esp. H-H RootRequestLease/cursor; H-A/H-C/
H-F/H-G; `regional-arch-pseudocode.md` §4c cursor lifetime). This is DESIGN: it
modifies no code; it pins the desired header shape as a DIFF from the CURRENT
collected headers in
`.../scratchpad/phase4/*.datalog.h`, quoting the current lines verbatim as the
before-state.

Witnesses:
- `demand_neighborhood_witness` — flat header (`.datalog.h`, 538 lines, flat arm
  `-demand -demand-retract`) AND nested header (`-nested.datalog.h`, 636 lines,
  adds `-demand-instance`). The nested header threads `InstanceStore` params;
  the desired state DELETES that threading.
- `demand_multi_adorn_witness-nested` — the two-adornment query surface
  (`q_bf` + `q_fb`), TWO `InstanceStore`s over one shared pub.
- `join_1` — must stay UNCHANGED (no demand). The byte-identity claim is stated
  precisely against the hidden-friend surface.

Grounding rule (house): every before-quote is copied verbatim from the collected
dumps. Line-exact where the surface is pinned (includes, ctor, hidden-friend
signatures, cursor struct, private members); shape-exact with metavariables
(`<id>`, `<edge>`, `RowReq`) where node/table ids drift.

Normative Stage-C anchors: `stage-c-diff.md` §H-A, §H-C, §H-F, §H-G, §H-H, §EG.6;
`RegionalDataFlowCore.md` §6 (leases), §7 (epoch), §10/§14 (retract/forcing
removal); `regional-arch-pseudocode.md` §4c.

---

## 0. Global determinism contract (header-wide, all witnesses)

The header is a PURE BYTE GOLDEN. Unlike `.rel`/`.stdout`, the header carries no
published-delta tokens, so **`permcheck.py` never applies to `.h`** — every
`.h.<mode>.golden` is byte-compared across all four optimization modes and
across the tagged pre-cutover / post-cutover referee (`stage-c-diff.md` §EG.3).

What is a pure function of the frozen regional program (hence deterministic and
byte-stable):

1. **Struct/table declaration order** — `Row*`, `Key*`, and the new
   request-edge row type emit in TableId order (Stage A `LogicalNodeId` order),
   never pointer-derived (HP-9). The `RootRequestLease`/request-edge additions
   slot at their owning node's position, not appended.
2. **Private-member order + ctor init-list order** — identical to each other and
   to member declaration order (the ctor init-list is a member-order walk).
3. **Hidden-friend emission order** — `init`, then message entries in
   kMessageHandler order, then query entries in `ProgramQuery` order
   (Database.cpp `:1507-1548`). Deleting the demand message entry (H-F) and the
   `*_retract` friend (H-H) removes lines but reorders nothing.
4. **Internal-proc order** — `init_<id>`, `proc_<id>`, `<msg>_<arity>_detail`,
   the acquire proc, `flow_<id>` — emitted in the frozen-region walk order that
   Stage B fixed; the demand `*_detail` and `inject_<id>` procs are deletions,
   not reorders.

The header carries **no census line** — the lifecycle census
(`V-LIFECYCLE-CENSUS`, H-G.4) lives in the `.rel` dump. The header's determinism
obligation is purely the four orderings above plus the byte-identity of every
surviving construct.

**Grep gate on the emitted header (`stage-c-diff.md` §EG.6):** the string
`demand__` MUST NOT appear anywhere; nor `InstanceStore`, `name_retract`,
`inject_`, `_retract`. Positive replacement tokens: `RootRequestLease`,
`AcquireRootRequestLease` (or the realized internal `acquire_<id>`).

---

## 1. `demand_neighborhood_witness` — the flagship flat→regional collapse

### 1.0 Orientation

`neighborhood/2` (bf) is a single-adornment bound query over a differential
(`add_edge`-driven, retractable) input. Flat arm today = a demand-message +
guard-web push-down join; nested arm = one keyed `InstanceStore`. Post-Stage-C
there is ONE regional path (H-A: no mode). The desired header is CLOSER to the
FLAT header than the nested one — the shared pub `neighborhood_15` already stores
answers once and the cursor already filters by `Start` via `idx_135` — with
three surgical changes: the demand message becomes a compiler-owned request-edge
relation, the retract entry point becomes the lease destructor, and the forcing
inject becomes lease acquisition. The nested header's `InstanceStore` threading
is DELETED outright (D8): no per-instance store in the Stage-C one-level
shared-pub realization.

### 1.1 Includes

Current — FLAT (`demand_neighborhood_witness.datalog.h:5-9`):

```cpp
#include <drlojekyll/Runtime/Allocator.h>
#include <drlojekyll/Runtime/Hash.h>
#include <drlojekyll/Runtime/Table.h>
#include <drlojekyll/Runtime/Vec.h>
```

Current — NESTED (`-nested.datalog.h:5-9`) additionally carries:

```cpp
#include <drlojekyll/Runtime/InstanceStore.h>
```

Desired (both arms collapse to one):

```cpp
#include <drlojekyll/Runtime/Allocator.h>
#include <drlojekyll/Runtime/Hash.h>
#include <drlojekyll/Runtime/Table.h>
#include <drlojekyll/Runtime/RootRequestLease.h>
#include <drlojekyll/Runtime/Vec.h>
```

**Contract.** `InstanceStore.h` is GONE (D8/E5 — the semantic keyed-instance
model is deleted; a physical per-instance store, if any, is a Stage-D addition).
`RootRequestLease.h` is a NEW runtime header (proposal §6) declaring the
move-only lease type; it is included by every header whose program has at least
one leased query root. Include order is the fixed alphabetical-within-group
codegen order, so `RootRequestLease.h` slots between `Hash.h`/`Table.h` and
`Vec.h` deterministically.

### 1.2 The demand-message backing table becomes the request-edge relation

Current — FLAT (`.datalog.h:30-37`), `table_4` is the backing table of the
fabricated `demand__neighborhood_bf/1` message (row = the demanded `Start`):

```cpp
// Rows of `table_4`.
struct Row4 {
  uint64_t c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row4 &) const noexcept = default;
};
```

Desired — the request-edge relation replaces it. Row schema carries the OWNER
identity that the demand-message row could not (F4 fix, H-F):

```cpp
// Rows of `request_edges_<id>` (root request edges for neighborhood/2 bf).
struct RowReq_<id> {
  uint64_t owner;   // RootLeaseId (proposal §5.2) — the exact requester
  uint64_t c0;      // the demanded key `Start`
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(owner, c0);
  }
  bool operator==(const RowReq_<id> &) const noexcept = default;
};
```

**Contract.** The demand-message row `(Start)` is presence-only and collapses
multiple requesters (F4). The request-edge row adds an `owner` column so two
cursors demanding the same `Start` are DISTINCT `RequestEdgeId`s and either can
retract independently. `ActiveInstanceRelation = DistinctProjection(child)` (§5.2)
projects `owner` away to recover today's presence semantics; `DemandSupportCount`
is the demoted count of live edges, never an owner.

**ADJUDICATION INPUT (A1).** The exact request-edge row schema is a design
choice this header cannot pin alone — minimally `{owner, Start}`, but a
`call_site` column may be needed if one query name carries multiple call sites
(it does not for a single-adornment query; it does for multi-adornment §2).
Whether `owner` is `uint64_t` or a Stage-A `RootLeaseId` newtype (which in the
generated header still lowers to a scalar) is a Stage-A/codegen decision. Flagged
for the owner.

### 1.3 DELETE the `InstanceStore` key/row structs (nested only)

Current — NESTED (`-nested.datalog.h:97-113`):

```cpp
// InstanceStore #0 key (the demanded alpha).
struct Key_0 {
  uint64_t c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Key_0 &) const noexcept = default;
};

// InstanceStore #0 published row.
struct Row_0 {
  uint64_t c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row_0 &) const noexcept = default;
};
```

Desired: DELETED entirely. No `Key_0`/`Row_0` instance-store structs exist. The
demanded row payload lives, as in the flat arm, in the ordinary pub
`neighborhood_15` (`Row15{start,node}`) — results stored ONCE (shared pub, H-G.2).

### 1.4 Internal-proc signatures: drop the `InstanceStore` param, retype the demand table

Current — FLAT declares seven procs threading `DiffTable<Row4> &table_4` (the
demand relation) but NO instance store, e.g. (`.datalog.h:115-116`):

```cpp
template <typename Log>
bool proc_19(::hyde::rt::Allocator &allocator, Log &log, ::hyde::rt::DiffTable<Row4> &table_4, ::hyde::rt::Table<Row7> &table_7, ::hyde::rt::Index<Key57> &idx_57, ::hyde::rt::DiffTable<Row11> &table_11, ::hyde::rt::Index<Key91> &idx_91, ::hyde::rt::DiffTable<Row15> &neighborhood_15, ::hyde::rt::Index<Key135> &idx_135, uint64_t &g20, ::hyde::rt::Vec<Tup_u64_u64> vec21, ::hyde::rt::Vec<Tup_u64> vec26, ::hyde::rt::Vec<Tup_u64> vec27);
```

Current — NESTED threads an EXTRA `InstanceStore<Key_0, Row_0> &instance_0` into
EVERY proc (`-nested.datalog.h:135`):

```cpp
..., uint64_t &g20, ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0, ::hyde::rt::Vec<Tup_u64_u64> vec21, ...);
```

Desired (shape-exact — internal-proc bodies are the H-G lowering author's
charge; here only the STATE THREADED changes):

```cpp
template <typename Log>
bool proc_19(::hyde::rt::Allocator &allocator, Log &log, ::hyde::rt::DiffTable<RowReq_<id>> &request_edges_<id>, ::hyde::rt::Table<Row7> &table_7, ::hyde::rt::Index<Key57> &idx_57, ::hyde::rt::DiffTable<Row11> &table_11, ::hyde::rt::Index<Key91> &idx_91, ::hyde::rt::DiffTable<Row15> &neighborhood_15, ::hyde::rt::Index<Key135> &idx_135, uint64_t &g20, ::hyde::rt::Vec<...> ...);
```

**Contract.** (a) The `InstanceStore<Key_0,Row_0> &instance_0` param is DELETED
from every internal proc signature (nested → desired). (b) `DiffTable<Row4>
&table_4` (the demand relation) is RETYPED to the request-edge relation
`DiffTable<RowReq_<id>> &request_edges_<id>`. (c) The `demand__neighborhood_bf_1_detail`
proc (flat `:118-120,:259-265`; nested `:139,:282-287`) is DELETED — there is no
demand message to receive; its `NetBatch(vec118, vec119)` two-fold body is
subsumed by the lease-edge net (`NetRootLeaseEdges`, §7). (d) The `inject_125` /
`inject_130` injector procs (flat `:121-124,:267-283`) are DELETED (see §1.7).

### 1.5 The database ctor + private members

Current — FLAT ctor (`.datalog.h:136-144`) + members (`:196-207`):

```cpp
  explicit Database(::hyde::rt::Allocator allocator_)
    : allocator(allocator_),
      table_4(allocator_),
      table_7(allocator_),
      idx_57(allocator_),
      table_11(allocator_),
      idx_91(allocator_),
      neighborhood_15(allocator_),
      idx_135(allocator_) {}
```
```cpp
  ::hyde::rt::DiffTable<Row4> table_4;
  ::hyde::rt::Table<Row7> table_7;
  ...
  ::hyde::rt::Index<Key135> idx_135;

  uint64_t g20 = 0;
  bool initialized_ = false;
```

Current — NESTED additionally constructs the store with the `monotone` bool
(`-nested.datalog.h:164`, `:226`):

```cpp
      instance_0(allocator_, false) {}
```
```cpp
  ::hyde::rt::InstanceStore<Key_0, Row_0> instance_0;
```

Desired ctor + members:

```cpp
  explicit Database(::hyde::rt::Allocator allocator_)
    : allocator(allocator_),
      request_edges_<id>(allocator_),
      table_7(allocator_),
      idx_57(allocator_),
      table_11(allocator_),
      idx_91(allocator_),
      neighborhood_15(allocator_),
      idx_135(allocator_) {}
```
```cpp
  ::hyde::rt::DiffTable<RowReq_<id>> request_edges_<id>;
  ::hyde::rt::Table<Row7> table_7;
  ...
  ::hyde::rt::Index<Key135> idx_135;

  uint64_t g20 = 0;
  bool initialized_ = false;
```

**Contract.** The `InstanceStore<...> instance_0` member and its
`instance_0(allocator_, false)` init are DELETED (D8). The `false` argument was
`monotone = !IsDifferential()` (`Database.cpp:1465-1473`); with no store, the
selector is gone. The demand backing table `table_4` becomes
`request_edges_<id>` (a small edge `DiffTable`). No new pending-removal-queue
member is required at the Database struct level if lease removals are staged in
the request-edge table's own delete queue; if a separate pending-lease vector is
needed it is a private member emitted here (**ADJUDICATION INPUT A2** — whether
the pending-lease-removal drain reuses the request-edge `kDeleteQueue` or needs a
dedicated member; proposal §6 "the next entry point nets all pending lease
removals").

### 1.6 DELETE the retract entry point

Current — FLAT (`.datalog.h:162-168`); NESTED identical modulo the threaded
`db.instance_0` (`-nested.datalog.h:182-188`):

```cpp
  // Query `neighborhood/2` (bf).
  // Retract a standing demand for `neighborhood/2` (bf).
  template <typename Log, typename Functors>
  friend void neighborhood_bf_retract(Database &db, Log &log, Functors &, uint64_t Start) {
    assert(db.initialized_);
    inject_130(db.allocator, log, db.table_4, db.table_7, db.idx_57, db.table_11, db.idx_91, db.neighborhood_15, db.idx_135, db.g20, Start);
  }
```

Desired: DELETED entirely — including the `// Retract a standing demand...`
comment line. There is NO `name_retract` friend (D6, H-H). Retraction is owned
by the cursor's lease destructor (§1.7). The `// Query neighborhood/2 (bf).`
comment survives, migrating to sit directly above the cursor struct.

### 1.7 The cursor gains a move-only lease; the forcing inject becomes lease acquisition

Current — FLAT cursor + factory (`.datalog.h:170-193`):

```cpp
  struct neighborhood_bf_cursor {
    Database &db;
    uint64_t Start;
    uint32_t pos;
    bool next(uint64_t &Node) {
      while (pos != ::hyde::rt::kNoRow) {
        const uint32_t id = pos;
        pos = db.idx_135.Next(id);
        const auto row = db.neighborhood_15.RowAt(id);
        if (!db.neighborhood_15.Present(id)) {
          continue;
        }
        Node = row.node;
        return true;
      }
      return false;
    }
  };
  template <typename Log, typename Functors>
  friend neighborhood_bf_cursor neighborhood_bf(Database &db, Log &log, Functors &, uint64_t Start) {
    assert(db.initialized_);
    inject_125(db.allocator, log, db.table_4, db.table_7, db.idx_57, db.table_11, db.idx_91, db.neighborhood_15, db.idx_135, db.g20, Start);
    return {db, Start, db.idx_135.First({Start})};
  }
```

Desired:

```cpp
  // A cursor over neighborhood/2 (bf). Owns a move-only RootRequestLease: the
  // lease holds the exact root RequestEdgeId this call added; its destructor
  // enqueues that edge's removal, netted before the next entry point. Copy is
  // deleted (the lease member is move-only). CONTRACT: drain this cursor fully
  // before the next database entry point — an intervening epoch's CompactDead()
  // may renumber `pos` (convention, not type-enforced; see below).
  struct neighborhood_bf_cursor {
    Database &db;
    uint64_t Start;
    uint32_t pos;
    ::hyde::rt::RootRequestLease lease;
    bool next(uint64_t &Node) {
      while (pos != ::hyde::rt::kNoRow) {
        const uint32_t id = pos;
        pos = db.idx_135.Next(id);
        const auto row = db.neighborhood_15.RowAt(id);
        if (!db.neighborhood_15.Present(id)) {
          continue;
        }
        Node = row.node;
        return true;
      }
      return false;
    }
  };
  template <typename Log, typename Functors>
  friend neighborhood_bf_cursor neighborhood_bf(Database &db, Log &log, Functors &, uint64_t Start) {
    assert(db.initialized_);
    auto lease = acquire_<id>(db.allocator, log, db.request_edges_<id>, db.table_7, db.idx_57, db.table_11, db.idx_91, db.neighborhood_15, db.idx_135, db.g20, Start);
    return {db, Start, db.idx_135.First({Start}), std::move(lease)};
  }
```

**Contracts.**

1. **`next()` body is byte-identical** to the flat arm (the `Present`-filtered
   `idx_135` walk). Under Stage C the shared pub `neighborhood_15` still stores
   answers once; the cursor still filters by `Start`. Nothing in the scan
   changes.
2. **The lease member.** `::hyde::rt::RootRequestLease lease;` is added as the
   LAST cursor member (after `pos`), so the aggregate initializer appends
   `std::move(lease)` as the fourth element. `RootRequestLease` is move-only
   (deleted copy) with a destructor that enqueues the exact
   `RequestEdgeId` removal into `db.request_edges_<id>` (a `kRequestEdgeRemove`,
   H-G.2). The cursor therefore INHERITS move-only-ness and a destructor from the
   member — the codegen need not emit an explicit `= delete` copy ctor or an
   explicit `~neighborhood_bf_cursor()`; the member's special members suffice.
   (**ADJUDICATION INPUT A3:** whether the codegen emits explicit
   `neighborhood_bf_cursor(const neighborhood_bf_cursor&) = delete;` +
   `neighborhood_bf_cursor(neighborhood_bf_cursor&&) = default;` for
   DOCUMENTATION/clarity, or relies on the implicit member-driven rules. Either
   is byte-stable; pick one for the golden.)
3. **The forcing call becomes lease acquisition.** `inject_125(...)` — which
   appended `Start` to the demand message's add-vector and CALLed the handler —
   is replaced by `acquire_<id>(...)` returning the lease. `acquire_<id>` is the
   realized internal proc (successor to `inject_<id>`): it enqueues the edge ADD
   (`kRequestEdgeAdd`) into `request_edges_<id>` and runs the epoch to net
   fixpoint (`EvaluateEpochToNetFixpoint`, §7) transitively, then returns a
   `RootRequestLease` naming the added `RequestEdgeId`. There is NO separate
   "run the epoch" step (same as today — the acquire IS the epoch run).
4. **The public arity is UNCHANGED.** `neighborhood_bf(Database &db, Log &log,
   Functors &, uint64_t Start)` keeps its exact `(db, log, functors, bound...)`
   signature. The lease is INTERNAL to the returned cursor — it does not appear
   in the friend's parameter list. `log`/`functors` remain because
   `acquire_<id>` runs an epoch that publishes to `log` and (for programs with
   functors) calls them. See §4 for the general forcing-function-arity rule.
5. **ADL/hidden-friend rules preserved.** `neighborhood_bf` stays a template
   hidden friend reachable only by unqualified ADL call with `db`;
   `neighborhood_bf_cursor` stays a nested struct; `acquire_<id>` is an internal
   (non-friend) template proc like `inject_<id>` was. The `assert(db.initialized_)`
   epoch-0 gate survives verbatim.

### 1.8 The `flow_136` publish tail (shape note)

Current — FLAT `flow_136` ends with the ordinary commit sweep over `table_4`,
`table_11`, `neighborhood_15` (`.datalog.h:506-535`); NESTED interposes the
`instance_0` band-(a1/a2/a2') full rescan + `instance_0.Seal()` +
`DebugValidate()` (`-nested.datalog.h:528-603`) BEFORE the ordinary sweep.

Desired: the nested arm's ENTIRE `instance_0` rescan/seal block
(`-nested.datalog.h:528-603`) is DELETED — no `FindOrAddInstance`,
`TouchCurrent`, `TryAdd`, `Touched()`, `V-INST-FRESH`/`V-INST-PARTITION` aborts,
`instance_0.Seal()`, or `DebugValidate()`. The region body is maintained by the
ordinary Rel differential fixpoint (`kLocalFixpoint`, H-G.2) that the flat arm
already emits; publication is gated by `RoutedResult` membership (edges ⋈ child)
rather than by an `InstanceStore` Present-probe. The commit sweep over
`neighborhood_15` (the shared pub) survives. This is the H-G author's charge in
detail; the HEADER-surface consequence is: the nested arm's `InstanceStore`
runtime calls VANISH from `flow_<id>`.

---

## 2. `demand_multi_adorn_witness-nested` — N adornments → N leases, one shared pub

### 2.0 Orientation

`#query q` carries TWO declared adornments (`q(bound A, free B)` = bf,
`q(free A, bound B)` = fb) over a non-recursive `rel(A,B)`. Today the nested arm
mints TWO disjoint `InstanceStore`s (`instance_0` keyed on A, `instance_1` keyed
on B) sharing ONE pub `q_4` (the O1/`(pub,forcing)` V-INST-SOLE relaxation; the
flagship `kSubgraphInstantiate=2`). Post-Stage-C: TWO leased query roots (one per
adornment friend `q_bf`/`q_fb`), each with its own `RootRequestLease`, riding ONE
shared pub `q_4` and ONE (or two) request-edge relation(s). The two
`InstanceStore`s are DELETED.

### 2.1 DELETE both `InstanceStore` key/row struct pairs

Current (`-nested.datalog.h:97-131`) declares `Key_0/Row_0` AND `Key_1/Row_1`
(the two demanded alphas). Desired: all four DELETED. The demanded payloads live
in the shared pub `q_4` (`Row4{a,b}`), routed per-adornment by request edges.

### 2.2 DELETE both demand detail procs; both `InstanceStore` params

Current — the two demand detail functions thread BOTH stores
(`-nested.datalog.h:152-153`):

```cpp
inline bool demand__q_bf_1_detail(::hyde::rt::Allocator &allocator, ..., ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0, ::hyde::rt::InstanceStore<Key_1, Row_1> &instance_1, ::hyde::rt::Vec<Tup_u64> vec43);
inline bool demand__q_fb_1_detail(::hyde::rt::Allocator &allocator, ..., ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0, ::hyde::rt::InstanceStore<Key_1, Row_1> &instance_1, ::hyde::rt::Vec<Tup_u64> vec48);
```

Desired: both `demand__q_bf_1_detail` / `demand__q_fb_1_detail` DELETED (no
demand messages). Every surviving internal proc DROPS the two
`InstanceStore<Key_0,Row_0> &instance_0, InstanceStore<Key_1,Row_1> &instance_1`
params. The `demand__q_*` receive/inject procs (`inject_56`, `inject_61`,
`.datalog.h:154-155,:313-325`) become the two `acquire_<id>` procs (§2.4).

### 2.3 The ctor loses BOTH store members

Current (`-nested.datalog.h:175-176`, `:251-252`):

```cpp
      instance_0(allocator_),
      instance_1(allocator_) {}
```
```cpp
  ::hyde::rt::InstanceStore<Key_0, Row_0> instance_0;
  ::hyde::rt::InstanceStore<Key_1, Row_1> instance_1;
```

Desired: both members and both init entries DELETED. (Note: THIS witness's stores
were constructed WITHOUT the `monotone` bool — `instance_0(allocator_)` — because
its pub `q_4` is a monotone `Table<Row4>`, contrasting neighborhood's
`instance_0(allocator_, false)`. Both spellings vanish.) Replacement: one shared
request-edge relation `request_edges_<id>` (or two, one per call site — A1/A4),
constructed with `allocator_`.

### 2.4 TWO query friends, TWO leases, ONE shared pub

Current — `q_bf` (`-nested.datalog.h:194-215`) and `q_fb` (`:217-238`) each have
a cursor + a friend that calls `inject_56` / `inject_61`. Both cursors scan the
SHARED pub `q_4` (bf via `idx_60` on `a`, fb via `idx_65` on `b`); NEITHER
cursor's `next()` re-checks `Present` because `q_4` is a monotone `Table` (no
deaths):

```cpp
  template <typename Log, typename Functors>
  friend q_bf_cursor q_bf(Database &db, Log &, Functors &, uint64_t A) {
    assert(db.initialized_);
    inject_56(db.allocator, db.q_4, db.idx_60, db.idx_65, db.table_8, db.table_11, db.table_19, db.g23, db.instance_0, db.instance_1, A);
    return {db, A, db.idx_60.First({A})};
  }
```

Desired (each adornment mirrors §1.7 independently):

```cpp
  struct q_bf_cursor {
    Database &db;
    uint64_t A;
    uint32_t pos;
    ::hyde::rt::RootRequestLease lease;
    bool next(uint64_t &B) { /* byte-identical idx_60 walk */ }
  };
  template <typename Log, typename Functors>
  friend q_bf_cursor q_bf(Database &db, Log &, Functors &, uint64_t A) {
    assert(db.initialized_);
    auto lease = acquire_bf_<id>(db.allocator, db.q_4, db.idx_60, db.idx_65, db.table_8, db.table_11, db.table_19, db.g23, db.request_edges_<id>, A);
    return {db, A, db.idx_60.First({A}), std::move(lease)};
  }
```

and symmetrically `q_fb` with `acquire_fb_<id>` + `idx_65`.

**Contracts.**

1. **N adornments → N leases, ONE pub.** Each adornment friend acquires its OWN
   `RootRequestLease` from its OWN call site; the two leases route through the
   shared pub `q_4`. This is the direct successor to `kSubgraphInstantiate=2` —
   two lease-acquiring roots replace two `InstanceStore`s. The `owner`/`call_site`
   columns of the request-edge relation distinguish the bf and fb roots
   (**ADJUDICATION INPUT A4:** one shared `request_edges_<id>` relation carrying
   a `call_site` discriminator, vs two per-adornment request-edge relations. The
   D3.a.3 shape was two disjoint stores → two relations is the literal
   translation; one relation with a call_site column is the consolidation. Owner
   decides; it changes the private-member count and the row schema).
2. **`next()` bodies byte-identical**, including the ABSENCE of a `Present` check
   (monotone `q_4`), contrasting neighborhood (differential pub → `Present`).
3. **This is a MONO carrier.** `demand_multi_adorn_witness` runs bare `-demand`
   in its flat arm (birth-only, no `-demand-retract`); under Stage C the flags
   are gone and both roots are UNIFORMLY retractable via their leases — the
   R-MONO special case collapses (H-G.3). The behavioral `.stdout` stays
   answer-identical; the header SHAPE is re-blessed.
4. Public arities `q_bf(db, log, functors, A)` / `q_fb(db, log, functors, B)`
   UNCHANGED; leases internal to the cursors.

---

## 3. `join_1` — UNCHANGED (the byte-identity claim, stated precisely)

### 3.0 Why it does not move

`join_1` compiles with NO demand flags (`join_1.orient.md:1`): it has no bound
`#query` that forces anything — `q/1 (f)` and `never/1 (f)` are ALL-FREE cursor
queries with no forcing function. Its header (`join_1.datalog.h`, 311 lines)
contains no `demand__` name, no `InstanceStore`, no `inject_`, no `*_retract`,
no forcing call — the query friends are the trivial forcing-free form:

```cpp
  friend q_f_cursor q_f(Database &db) {
    assert(db.initialized_);
    return {db, 0};
  }
```

(`join_1.datalog.h:183-186`; `never_f` identical at `:202-205`). Note these
friends are NOT even templated on `Log`/`Functors` (no forcing → no epoch to run
→ no `log`/`functors` needed); the cursor is `{db, 0}` with a bare `uint32_t pos`
scanning `q_6`/`never_9` directly.

### 3.1 The precise byte-identity claim

`join_1.datalog.h` is **byte-identical before and after Stage C**, in all four
optimization modes. The claim rests on THREE facts, each tied to the
hidden-friend surface:

1. **No deletion touches it.** Every H-A/H-C/H-F/H-G/H-H deletion targets a
   demand/instance/retract/forcing construct. `join_1` has NONE:
   `grep -E 'demand__|InstanceStore|inject_|_retract|forcing|RootRequestLease'`
   over the current header is EMPTY, so no line is removed and no line is added
   (no lease include, no lease member — there is no leased root).
2. **No forcing-free cursor changes.** The Stage-C cursor change (§1.7) adds a
   `RootRequestLease` member ONLY to cursors of LEASED (forcing) query roots. A
   forcing-FREE query (`q_f`, `never_f`) acquires no lease — its cursor keeps the
   bare `{db, pos}` shape and its friend keeps the non-templated `q_f(Database
   &db)` signature. The proposal's `RegionalCursor<Row>` lease field is
   conditional on a root lease existing; join_1's roots are neither leased query
   roots nor demand-forced.
3. **The regional path is representation-only for non-demand programs (H-A
   golden consequence).** Extraction is answer-neutral; a program with no
   extractable pure demanded child produces the same local-graph lowering it does
   today. `join_1`'s two acyclic pivot-joins (`kEagerJoin=4`, `join_1.orient.md`)
   are region-body ops that survive unchanged (MECH "carried forward"). The
   header — tables `q_6/never_9/table_12/16/19/23`, indexes `idx_39/idx_45`,
   procs `init_3/proc_26/t1_2_detail/t2_1_detail/flow_60`, and the two
   forcing-free cursor friends — is emitted identically.

**Determinism corollary.** Because `join_1` is a pure representation no-op, it is
the NEGATIVE witness for the header determinism contract: if any Stage-C code
path perturbs a non-demand header (member reorder, spurious lease include, cursor
struct churn), `join_1.datalog.h` diverges and the golden catches it. It belongs
in the re-bless set ONLY as a byte-identity assertion (its golden MUST NOT
change), analogous to the `.rel` `kSubgraphInstantiate=0` census pins.

---

## 4. Cross-cutting: the forcing-function signature family

The charge asks whether the lease changes the public arity of the
queries-with-a-forcing-function family `(db, log, functors, bound...)`.

**Answer: NO. The lease does not change any public arity.** Precisely:

- A FORCING query (neighborhood_bf, q_bf, q_fb, demand_tc_witness's query, …)
  keeps its exact `friend <ret> name_<pattern>(Database &db, Log &log, Functors
  &, bound...)` signature. `log`/`functors` remain in the signature because
  `acquire_<id>` runs an epoch (§7 EvaluateEpoch) that publishes to `log` and
  invokes `functors` — the same reason the current forcing call needs them. The
  `RootRequestLease` the acquire returns is CONSUMED INTERNALLY into the cursor's
  aggregate initializer; it never appears as a parameter.
- A FORCING-FREE query (join_1's `q_f`/`never_f`, and any `#query` whose
  materialization is unconditional) keeps its trivial `friend q_f_cursor
  q_f(Database &db)` form — no `Log`/`Functors` template, no lease.
- The DIFFERENCE from today is not the arity but the BODY: `inject_<id>(...)` →
  `acquire_<id>(...) → RootRequestLease`, plus the `std::move(lease)` factory
  argument. Byte-for-byte, the friend's declarator line is unchanged; only its
  two-line body changes.

**All-bound existence-check queries (`!has_free`) — ADJUDICATION INPUT A5.**
None of the three witnesses carries an all-bound query, but the family exists
(`stage-c-diff.md` H-H "existence check (Find/Present) unchanged shape";
`Database.cpp:1706-1724`). An all-bound forcing query returns `bool`, not a
cursor, so there is no cursor to OWN the lease. The Stage-C shape must acquire a
FUNCTION-SCOPED lease (acquire → EvaluateEpoch → Find/Present → the lease's
destructor fires at return, enqueuing the removal for the next entry). This is a
lease with a DIFFERENT lifetime than the cursor-owned lease (scoped, not moved
out). Flagged because it is the one forcing shape whose lease is NOT
cursor-owned, and no witness pins it — the exit gate (§EG) should author or
identify one.

---

## 5. The drain-before-next-entry contract: how it surfaces

The charge asks: comment, assert, or type? **It surfaces as a COMMENT, not an
assert and not a type** (proposal §6; `stage-c-diff.md` H-H bullet 3; E7).

- **What the lease DOES type-close:** the demand-vs-cursor SPLIT (F3). The two
  structurally-unconnected APIs of §4c ("demand lifetime" = InstanceStore
  touch/rebuild + demand-message row presence; "cursor lifetime" = a raw
  `uint32_t pos`) become ONE move-only token. RETRACTION is now type-safe: the
  lease destructor enqueues the exact edge removal; there is no way to leak or
  double-retract a demand, and there is no user-invoked `name_retract` to call
  out of order (D6).
- **What the lease does NOT type-close:** cursor staleness against an intervening
  `CompactDead()` renumber. `pos` is still a raw index into `db.neighborhood_15`;
  an intervening entry point (another message, another query) can run a commit
  sweep whose `CompactDead()` renumbers the very rows `pos` indexes, with NO
  generated guard. The first implementation keeps drain-before-next-entry as
  CONVENTION (proposal §6 "The first implementation enforces this existing
  drain-before-next-entry contract").
- **Header realization:** a documentation comment on the cursor struct (§1.7
  desired block) states the contract in prose. There is NO added `assert` in
  `next()`, and NO epoch-version field on the cursor to detect a renumber. The
  existing CLAUDE.md cursor contract ("drain fully before the next entry-point
  call; keyed-cursor order unspecified") continues to bind by convention.

**Do not let the exit gate over-claim "F3 fully closed"** (E7): the lease closes
the demand/cursor split and makes retraction exact; cursor staleness against an
intervening epoch remains conventional in Stage C.

---

## 6. Summary — desired-state contract, determinism, adjudication inputs

### Desired-state contract (what the Stage-C header looks like)

- **Includes:** drop `InstanceStore.h`; add `RootRequestLease.h` for any program
  with a leased query root.
- **Structs:** the demand-message backing table row (`Row4`) becomes a
  request-edge row `RowReq_<id>{owner, key...}`; all `Key_N`/`Row_N`
  InstanceStore struct pairs are DELETED.
- **Internal procs:** drop the `InstanceStore<...> &instance_N` param(s) from
  every signature; retype the demand `DiffTable` to the request-edge `DiffTable`;
  DELETE the `demand__*_detail` receive procs and the `inject_<id>` procs;
  ADD the `acquire_<id>` proc(s) returning a `RootRequestLease`.
- **Database ctor/members:** DELETE the `instance_N(allocator_[, monotone])`
  init(s) and the `InstanceStore` member(s); the demand backing table becomes
  the request-edge relation.
- **Friends:** DELETE the `name_retract` friend + its comment; every forcing
  query friend KEEPS its `(db, log, functors, bound...)` arity, its
  `assert(db.initialized_)`, and its hidden-friend/ADL nature — only its body's
  `inject_<id>` → `acquire_<id>` + `std::move(lease)`.
- **Cursor:** the per-query nested cursor struct gains a `::hyde::rt::RootRequestLease
  lease;` LAST member (move-only ⇒ deleted copy, destructor enqueues the exact
  `RequestEdgeId` removal, inherited from the member); `next()` byte-identical; a
  prose comment carries the drain-before-next convention.
- **N adornments → N leases, one shared pub** (multi_adorn): two leased query
  friends replace two InstanceStores; behavioral `.stdout` invariant, header
  shape re-blessed.
- **join_1 byte-identical** in all four modes: no demand ⇒ no deletion touches
  it, forcing-free cursors gain no lease, and the regional path is a pure
  representation no-op for non-demand programs. It is the negative determinism
  witness (golden MUST NOT change).

### Determinism contract

The header is a pure byte golden (no `permcheck.py` — no published-delta tokens
in `.h`). Struct order, private-member order, ctor init-list order,
hidden-friend order, and internal-proc order are all pure functions of the frozen
regional program's TableId/`LogicalNodeId` walk (never pointer-derived, HP-9).
The header carries no census line (that lives in `.rel`). Every surviving
construct is byte-identical; deletions remove lines without reordering.

### Adjudication inputs flagged

- **A1 — request-edge row schema.** `{owner, key}` minimal; whether a `call_site`
  column and whether `owner` is a Stage-A newtype vs bare `uint64_t`. (§1.2)
- **A2 — pending-lease-removal drain storage.** Reuse the request-edge
  `kDeleteQueue`, or a dedicated private pending-lease member. (§1.5)
- **A3 — explicit vs implicit cursor special members.** Emit
  `= delete` copy / `= default` move + explicit `~cursor()` for documentation,
  or rely on the move-only lease member's implicit rules. Byte-stable either way;
  the golden must pick one. (§1.7)
- **A4 — one shared request-edge relation with a call_site discriminator vs N
  per-adornment relations** for a multi-adornment query name. Changes
  private-member count and row schema. (§2.4)
- **A5 — all-bound existence-check lease.** The `!has_free` query returns `bool`,
  has no cursor to own the lease ⇒ needs a function-scoped lease with a
  destruct-at-return lifetime. No witness pins it; the exit gate should
  author/identify one. (§4)
- **(Inherited) E1** — the differential-published-answer arm (neighborhood's
  differential pub) is corpus-UNEXERCISED pre-cutover (`DRInstance::differential
  == false` today); the tagged binary cannot adjudicate the header shape's
  runtime behavior for the differential-pub lease. I0 must carry that case.
  `demand_neighborhood_witness` is the differential-pub HEADER carrier but not a
  pre-cutover behavioral oracle for the `if (diff)` arm.
