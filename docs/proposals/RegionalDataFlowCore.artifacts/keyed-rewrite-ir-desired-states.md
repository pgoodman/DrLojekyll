# Keyed-instance rewrite — desired IR output states for the carriers (phase-staged)

Session 12 (2026-08-06, tip `46a404d4`). Deliverable D4: the DESIRED post-cut IR
output states for the `key_*`/`demand_*` carrier datasets, across every IR surface
(`.df`/`.contract`, `-region-out` + DOT twin, `.rel`, generated header, generated
C++), staged per phase. Predict-then-verify discipline:

- **Baselines are EMPIRICALLY VERIFIED** at tip (compiled read-only into
  `scratchpad/dumps`; the suite stays `SUITE: PASS` — no production code touched).
- **Post-cut states are PREDICTIONS.** They cannot be empirically verified this
  session because implementing P1 is owner-gated. Each becomes a golden to
  re-bless (via `runall.sh --bless`, never automatically) when its phase lands.
  This doc IS the predict half; the verify half runs at implementation.

Carriers (per `next-session-prompt.md` "Test migration"):
- **`key_neighborhood_witness`** — non-recursive nested flagship (`kSubgraphInstantiate=1`).
  P4 (honest complete-path) carrier.
- **`key_tc_witness`** — recursive, flat-fallback (`kSubgraphInstantiate=0` but demand
  still present). P6 (recursive regional execution) carrier.
- **`key_multi_adorn_witness`** — two disjoint stores one pub (`kSubgraphInstantiate=2`).
  P5 (partial-binding DAG / convergence) carrier.

The `demand_*` twins (`demand_tc_witness`, `demand_neighborhood_*`, `demand_multi_adorn_witness`)
lose their raison d'être at P1: they exist to witness pragma-activation ≡
flag-activation byte-identity (verified at tip: every surface identical except the
case-name-bearing contract line). Once `-demand` is deleted, the equivalence is
meaningless; per Test migration they are DELETED as architectural requirements,
their answer-checking role folded into the surviving `key_*` datasets.

---

## §1. The cross-surface phase-change matrix (what each phase touches)

| Surface | P1 (cut) | P2 (typed) | P3 (request/activation) | P4 (honest path) | P5 (DAG) | P6 (recursive) |
| --- | --- | --- | --- | --- | --- | --- |
| `.df` | demand JOINs, GuardAnnotation `tag=`, RecognizedSubgraph render (`Format.cpp:1747`) GONE | — | — | — | — | — |
| `.contract` | `inferred=` drops; `declared=` stays inert | member_key from typed RelationSchema (same bytes — **silent-pass risk F11/F12**) | — | `declared=` becomes ordered path (consumed) | binding-state ids appear | — |
| `-region-out` | `request-port`→0, `region-internal demand__` GONE, query→`permanent-root`, port indices shift | bytes UNCHANGED, provenance typed (**F11 silent-pass**) | `request-port` RETURNS typed (`owner=RootLease edge=RE#`) | keyed access line honest | binding-state block added | recursive-component block added |
| `.rel` | `instances:`/`kSubgraphInstantiate`/`kInstanceSeal`/`demand__` kIngestFold GONE | census recount (demand-free) | RequestEdge/RuleActivationEdge ops | `kAccess(...,FullScanFilter)` replaces `section-walk` | BindingEdge ops | joint-fixpoint round shells |
| header | `InstanceStore<>`, `Key_/Row_` structs GONE | — | RegionalCursor typed | keyed cursor honest | — | — |
| C++ | `SUBGRAPH_INSTANTIATE` region, `emit_instance_rescan` GONE; plain query cursor | — | request-edge routing | full-scan-with-key-filter, honestly labelled | — | semi-naive worklist |

Two surfaces the phase exit-gates must NOT leave unpinned (critique F26): the
`.rel` census multiset and the `-region-out` census line. Both are pinned by the
per-carrier `.irgold` sidecars below.

---

## §2. key_neighborhood_witness (P4 flagship) — full phase-staged states

### 2.0 Baseline @ tip — VERIFIED

```
# .rel (excerpt)
instances:
  DRInstance i#0 forcing=neighborhood key=%table:8 pub=%table:4 input=%table:11 store=I#0 key_cols=[From] row_cols=[To]
op.0 kSubgraphInstantiate ... spine: kAccess(%table:11, section-walk) -> kFold(%table:4,+,NonRecursive)
op.5 kIngestFold ... message=demand__neighborhood_bf/1
op.1 kInstanceSeal ...
# census: kSubgraphInstantiate=1 kInstanceSeal=1 kIngestFold=2 kCommitSweep=2

# -region-out
  request-port     P0  query=neighborhood  fields=(Start)
  input-port       P1  message=add_edge/2  fields=(From, To)
  region-internal  demand__neighborhood_bf/1(p0:u64)  [fabricated, driver-suppressed]
  row-contract     E0  rel=neighborhood  member-key=(Start, Node)  support=monotone
  row-contract     E1  rel=edge          member-key=(From, To)     support=monotone
census: regions=1 child-calls=0 program-roots=1 request-ports=1 input-ports=1 result-ports=0 row-contracts=2

# .contract (tail)
declared-key rel=edge declared=(From) inferred=(From)
```

### 2.1 Post-P1 (demand deleted; the honest baseline) — PREDICT

```diff
# .rel
-instances:
-  DRInstance i#0 forcing=neighborhood key=%table:8 pub=%table:4 input=%table:11 store=I#0 ...
-op.0 kSubgraphInstantiate ... spine: kAccess(%table:11, section-walk) -> kFold
-op.5 kIngestFold ... message=demand__neighborhood_bf/1
-op.1 kInstanceSeal
+# edge materialized by the ordinary eager web; neighborhood reads it via a plain
+# query cursor. ONE ingest fold (add_edge/2). census kSubgraphInstantiate=1->0,
+# kInstanceSeal=1->0, kIngestFold=2->1.

# -region-out
-  request-port     P0  query=neighborhood  fields=(Start)
-  input-port       P1  message=add_edge/2  fields=(From, To)
-  region-internal  demand__neighborhood_bf/1(p0:u64)  [fabricated, driver-suppressed]
+  input-port       P0  message=add_edge/2  fields=(From, To)
+  permanent-root   neighborhood(Start, Node)
   row-contract     E0  rel=neighborhood  member-key=(Start, Node)  support=monotone
   row-contract     E1  rel=edge          member-key=(From, To)     support=monotone
-census: ... request-ports=1 input-ports=1 result-ports=0 row-contracts=2
+census: ... request-ports=0 input-ports=1 result-ports=0 row-contracts=2

# .contract
-declared-key rel=edge declared=(From) inferred=(From)
+declared-key rel=edge declared=(From)     # inert; no demand inference exists
```
- header/C++: NO `InstanceStore`, NO `SUBGRAPH_INSTANTIATE`, NO `emit_instance_rescan`.
  The bound query `neighborhood_bf(db, Start)` returns a plain cursor over the
  materialized `edge` table.
- **ANSWER unchanged.** neighborhood(Start) = { Node : edge(Start,Node) } — now by
  reading the full materialized relation. This is the one honest baseline.
- **`demand_neighborhood_*` twins DELETED** (their eqgate/pragma-activation role is gone).

### 2.2 Post-P2 (typed) — PREDICT: bytes IDENTICAL, provenance typed

The `-region-out`/`.contract` bytes are byte-identical to 2.1. The member_key/support
now derive from typed `RelationSchema` records, not string-copied `RowContract`.
**CRITIQUE F11/F12 (silent-pass):** a byte-identical golden CANNOT distinguish P2's
typed source from P1's string source, and P2's census exit-gate becomes a tautology
(census projects the very records it should independently recount). REQUIRED
DISCRIMINATING PROBE for the P2 exit gate (fold into this doc's golden set):
- a `-region-out -region-debug` mode (or a DEBUG assert) that dumps each contract's
  `origin=typed-schema#<id>` so the golden proves the render came from the typed
  record; AND
- the census recount uses an INDEPENDENT counter (walk the typed records vs count
  the rendered lines) so a stub that emits neither still fails.

### 2.3 Post-P3 (request/activation) — PREDICT: request port returns, typed

```diff
# -region-out
+  request-port     P0  query=neighborhood  fields=(Start)  owner=RootLease  edge=RE#0
   input-port       P1  message=add_edge/2  fields=(From, To)
-  permanent-root   neighborhood(Start, Node)
+  # neighborhood routed through RE#0 to its EMPTY-binding state (P3: only empty state exists)
-census: ... request-ports=0 ...
+census: ... request-ports=1 ...   # typed RequestEdgeId, NOT a demand forcing
```
- The `region-internal demand__` line does NOT return (no fabricated message).
- `.rel` gains `kRequestEdge`/`kRouteResult` ops (new vocabulary) over the empty state.
- A late 2nd requester attaches `RE#1` to the same dest state — `edge`/`neighborhood`
  facts are NOT duplicated (retained: caller-qualified results). Removing `RE#0`
  retracts only `RE#0`'s routed copies.

### 2.4 Post-P4 (honest complete-path) — PREDICT: keyed access, honest label

```diff
# .rel
+op.K kAccess(%table:edge, FullScanFilter) bound=[From=Start] -> kFold(neighborhood,+,NonRecursive)
#      ^^^ FullScanFilter, NOT section-walk. V-PLAN-HONEST: label == the emitted
#          full-scan-with-key-filter code.
# .contract: declared-key rel=edge declared=(From)   # now CONSUMED — drives BindingStateId for path [From]
```
- **CRITIQUE B1 (reintro-gap):** P4 must add a REAL emission hunk — a ControlFlow
  region + a `Database.cpp` region-dispatch arm + an Emit function + a DR-op
  lowering — not prose. The desired C++ is the full-scan-with-key-filter shape (the
  byte-shape may match the old `emit_instance_rescan` body MINUS the InstanceStore
  double-buffer; the honest difference is the label + that it reads/writes the
  canonical relation, not a per-key leaf cache).
- **CRITIQUE B2 (silent-pass):** the P4 exit gate must include a STRUCTURAL probe
  that the keyed access node EXISTS and is labelled `FullScanFilter` and routes
  through the P3 RequestEdge — because ANSWER-equality alone is satisfied by the P1
  baseline with `@key` inert. Reuse `key_neighborhood_witness` as an ANSWER witness
  AND add a `.rel` structural pin (`kAccess(...,FullScanFilter)` present).

### 2.5 Post-P5 — PREDICT: single-key, no convergence to witness here

`@key(From)` interns ONE `{From}` BindingStateSchema + one `empty --From--> {From}`
BindingEdge (new `-region-out` binding-state block). No path-convergence for a
single key — that is `key_multi_adorn_witness`'s job (§4).

### 2.6 Phases 6–9: NO CHANGE (edge is not in an SCC; single scan plan).

---

## §3. key_tc_witness (P6 recursive carrier) — states at the load-bearing phases

Baseline: recursive `path(F,T):edge_2; path(F,M),edge_2(M,T)`; the pragma took the
SILENT FLAT FALLBACK (`kSubgraphInstantiate=0`) but demand is present:
`region-internal demand__reachable_from_bf/1`, `request-port P0`,
`declared-key rel=path declared=(From) inferred=(From)`, and a
`kIngestFold ... message=demand__reachable_from_bf/1` in `.rel`.

### 3.1 Post-P1 — PREDICT
```diff
# -region-out
-  request-port     P0  query=reachable_from  fields=(From)
-  region-internal  demand__reachable_from_bf/1(p0:u64)  [fabricated, driver-suppressed]
+  input-port       P0  message=edge_2/2  fields=(From, To)
+  permanent-root   reachable_from(From, To)
-census: ... request-ports=1 ... row-contracts=2
+census: ... request-ports=0 ... row-contracts=2
# .contract:  declared-key rel=path declared=(From)   # inert; inferred= dropped
# .rel: the demand__ kIngestFold GONE; `path` is the ordinary recursive INDUCTION
#       fixpoint (kEagerJoin=8, kJoinEmit=5 etc. UNCHANGED — TC always lowered flat).
```
The recursive TC now materializes the FULL `path`/`reachable_from` relation
(unbound-complete), and the bound query reads it. **This is the honest baseline that
the P6 phases specialize.** ANSWER unchanged (reachable_from(From) is a subset of the
full closure; post-P1 the full closure is materialized and filtered at read).

### 3.2 Post-P6 — PREDICT: joint rooted-reachability over {From} binding states

The flat full-closure materialization is replaced by a joint fixpoint over `{From}`
binding states. `.rel` gains recursive-component + activation-edge round shells.
- **CRITIQUE B0 (blocking soundness, P6.5):** the worklist MUST carry the differential
  OVERDELETE→REDERIVE→INSERT machinery (retained deletion contract), not add-only
  semi-naive — else a retracted `edge_2` that removes support for a still-rooted
  `path` fact is never retracted. The desired `.rel` must show the signed-counter
  fixpoint (the existing `UPDATECOUNT` zero-crossing shape), keyed per binding state.
- **CRITIQUE H6 (P6.1):** the recursive-component SCC must close through the
  INSERT-to-stream (message publish/receive) seams the way `Stratify` does, or a
  message-mediated recursion is mis-partitioned. Desired: `recursive_components`
  agrees with `Stratify`'s condensation on every corpus program.
- co-recursive `p(K,X)@key(K) / q(X,K)@key(X)` (a NEW directed carrier to add): the
  `(p,{K})→(q,{X})→(p,{K})` activation cycle appears in `.rel`; `RequestEdge` stays
  acyclic (retained). Exit: different-key co-recursion converges to the P1 baseline's
  published surface, byte-identical across 4 opt modes.

---

## §4. key_multi_adorn_witness (P5 convergence carrier)

Baseline: `@key(A) @key(B)` → TWO disjoint stores one pub (`kSubgraphInstantiate=2`);
`request-port P0 adorn=bf`, `P1 adorn=fb`; two `region-internal demand__q_bf/1`,
`demand__q_fb/1`; `declared-key rel=rel declared=(A) inferred=(A)` +
`declared=(B) inferred=(B)`.

### 4.1 Post-P1 — PREDICT
```diff
# -region-out
-  request-port     P0  query=q  adorn=bf  fields=(A)
-  request-port     P1  query=q  adorn=fb  fields=(B)
-  region-internal  demand__q_bf/1(p0:u64)  [fabricated, driver-suppressed]
-  region-internal  demand__q_fb/1(p0:u64)  [fabricated, driver-suppressed]
+  input-port       P0  message=edge_2/2  fields=(A, B)
+  permanent-root   q(A, B)                          # both adornments -> one permanent root
-census: ... request-ports=2 ... row-contracts=2
+census: ... request-ports=0 ... row-contracts=2
# .contract:
-declared-key rel=rel declared=(A) inferred=(A)
-declared-key rel=rel declared=(B) inferred=(B)
+declared-key rel=rel declared=(A)                  # both inert; no inference
+declared-key rel=rel declared=(B)
# .rel: instances: block (i#0, i#1) + kSubgraphInstantiate=2 + kInstanceSeal=2 GONE;
#       `rel` materialized once; both queries read it. kIngestFold 3->1.
```
- **`demand_multi_adorn_witness` twin DELETED.**

### 4.2 Post-P5 — PREDICT: the convergence witness (goals 2 + 6)

This is the carrier that proves the partial-binding DAG. Desired `-region-out`
binding-state block for `rel` with `@key(A) @key(B)`:
```
  binding-schema  S0  rel  fields={}          # empty
  binding-schema  S1  rel  fields={A}
  binding-schema  S2  rel  fields={B}
  binding-edge    empty --A--> S1
  binding-edge    empty --B--> S2
```
And the SHARED-PREFIX / CONVERGENCE assertions (the P5 required properties), pinned
by adding a `@key(A) @key(A,B)` sibling probe:
- `@key(A)` and `@key(A,B)` share ONE `{A}` schema id (F: one `binding-schema
  fields={A}`, one `empty--A-->{A}` edge — NOT two).
- `[A,B]` and `[B,A]` (a `@key(A,B) @key(B,A)` probe) share their FINAL `{A,B}`
  schema id via two distinct edges (`{A}--B-->{A,B}` and `{B}--A-->{A,B}`).
- **CRITIQUE F7/H10:** the exit gate must NOT check "no `{A,B}` state for
  `@key(A,B,C)`" (a declared prefix IS materialized); it must check that
  UNVISITED, UNDECLARED subsets are absent, and that order-free schema identity
  is NOT re-collapsed by a sort in `BindingStateId` (the P8 `TrieNode` regression).

---

## §5. Self-critique of these desired states (deepen→diff→critique closure)

Applying the same refute discipline to D4 itself:

1. **Every post-cut block is a PREDICTION, not a verified golden.** Mitigation:
   each is diff-shaped against a VERIFIED baseline, so the verify step at
   implementation is a byte-diff, not a re-derivation. The baselines were compiled
   read-only this session; suite green.
2. **The P2 "bytes identical" prediction is a hazard, not a feature** (F11). This doc
   makes it explicit and requires a discriminating probe — a byte-identical golden
   is INSUFFICIENT as a P2 exit gate. Recorded as a P2 obligation.
3. **Port-index shift (P1) re-pads/re-numbers.** The `-region-out` port indices are
   dense and request-ports-first; deleting request ports renumbers `input-port
   P1→P0`. Predicted above; a golden bless at P1 must expect the renumber (not just
   line deletion). Analogous to the E-K5-PAD member-key width hazard.
4. **The `.rel` census multiset is the strongest structural pin.** For every carrier
   the `kSubgraphInstantiate`/`kInstanceSeal`/`kIngestFold` deltas are exact
   integers — a stubbed P1 that leaves any instance op raises the census and fails.
   This is the anti-silent-pass belt for P1 (stronger than the `-region-out`
   byte-golden).
5. **New carriers required** that don't exist at tip: the co-recursive
   `p@key(K)/q@key(X)` program (P6), and the `@key(A) @key(A,B)` / `@key(A,B) @key(B,A)`
   convergence probes (P5). Listed as test-authoring obligations for those phases;
   they have NO baseline (they're new), so their desired states are pure targets.
6. **Answer-equality is necessary but not sufficient** for P4/P5/P6 (B2). Every
   reconstruction-phase carrier needs a STRUCTURAL `.rel`/`-region-out` pin ALONGSIDE
   the answer/oracle witness, because the P1 full-materialization baseline already
   produces the right answers with `@key` inert.

---

## §6. NEW carrier — co-recursive `p(K,X)@key(K) / q(X,K)@key(X)` (P6, no baseline)

The different-key co-recursion witness (roadmap test 11; `next-session-prompt.md`
"Co-recursive key flow"). Carrier `.dr` (to land in `tests/OptDiff/cases/` when P6
arrives — NOT added this session):

```datalog
#message seed_p(u64 K, u64 X).
#local p(u64 K, u64 X) @key(K).
#local q(u64 X, u64 K) @key(X).
p(K, X) : seed_p(K, X).
p(K, X) : q(X, K).
q(X, K) : p(K, X).
#query query_p(bound u64 K, free u64 X) : p(K, X).
```

### 6.0 Current behavior @ tip — VERIFIED (compiled read-only this session)

**REJECTS**, rc=1: `error: Unsupported rule-body shape under -demand; fix or remove
the @key pragma` (anchored at the module head). The `@key` pragma force-activates the
demand transform (RP-6), whose body walk rejects the mutual `p:-q / q:-p` recursion —
exactly the `demand_cyclic_1`/`demand_mutual_content_1` fence class. **There is no
`.rel`/`-region-out` baseline to diff against** — this is a pure target: the program
that must go from REJECT to a converging keyed fixpoint is the whole point of P6.

### 6.1 Post-P1 — PREDICT: compiles as flat full materialization

`@key` inert; the mutual recursion lowers as an ordinary differential INDUCTION SCC
over the fully-materialized `p`/`q` relations; `query_p_bf` reads `p` via the plain
cursor. `.rel` census: `kSubgraphInstantiate=0`, ordinary `kEagerJoin`/`kJoinEmit`/
INDUCTION round shells (like `key_tc_witness` post-P1). `-region-out`: `request-ports=0`,
`permanent-root query_p`, two `input-port`/interior contracts for `p` and `q`. **This is
the honest baseline the P6 phases specialize** — and the answer/oracle referee for every
later phase (fresh-from-committed least fixpoint of `p`,`q`).

### 6.2 Post-P6 — PREDICT: joint fixpoint over differently-keyed binding states

Structural pins (predict-then-verify targets; STRUCTURAL, not answer-only per B2):

```
# -region-out (recursive-component block, P6.1)
  recursive-component  C0  { p, q }          # ONE component spanning BOTH relations
                                              # (D2/F16: sole populator P6.1; F7: closes
                                              #  the seed_p publish->receive seam)
  binding-schema  S_p  p  fields={K}          # p keyed on K
  binding-schema  S_q  q  fields={X}          # q keyed on X (DIFFERENT key — legal, goal 7)
# .rel — the key-changing activation CYCLE between binding states (P6.4):
  activation-edge (p,{K}) --q:-p--> (q,{X})   # re-keys p's fact through q's declared path
  activation-edge (q,{X}) --p:-q--> (p,{K})   # ... and back; a CYCLE in activation_edges
# request-edges STAY ACYCLIC (retained): query_p owns RE#0 -> (p,{K}); no RequestEdge cycle.
# .rel census: a JointFixpoint round-shell family (P6.5) keyed per binding state, with the
#   split C_nr/C_r counters (P6.5 pass (A)) present — NOT an add-only loop.
```

Exit gate (from reconstruction-diffs §3 P6.3-P6.6): (1) `recursive_components == {p,q}` as
ONE component, cross-checked against Stratify's projected condensation (F7/F19). (2)
Answer CONVERGES to the P1 baseline's published surface, byte-identical across 4 opt modes
+ `bin/Oracle` + I0 `RefInterp`, worklist-order-perturbation invariant (P6.5). (3) The
activation graph contains the `(p,{K})→(q,{X})→(p,{K})` cycle while `V-OWNERSHIP-ACYCLIC`
(RequestEdge forest) still holds — goal 3 witnessed structurally. (4) **B0/F1 deletion
witness:** retract a `seed_p` row that (transitively) supports the still-rooted `p`/`q`
cycle → the derived facts ARE retracted (pass (A) DRed), with a directed assertion that
`FactDerivation.support > 0` at removal yet the fact drops (reachability/DRed decided it,
not a counter). A `.batches` + `.probes` witness carries the birth/retract phases.

---

## §7. NEW carriers — P5 convergence probes (no baseline)

Two probes for the partial-binding DAG (roadmap tests 8/9). Neither exists at tip; both
REJECT today (verified read-only this session), for DIFFERENT reasons that pin the two
distinct P5/P0 corrections.

### 7A. Prefix-sharing — `@key(A) @key(A,B)` (roadmap test 8)

```datalog
#message edge_2(u64 A, u64 B).
#local rel(u64 A, u64 B) @key(A) @key(A, B).
rel(A, B) : edge_2(A, B).
#query q(bound u64 A, free u64 B) : rel(A, B).
```

**Current @ tip — VERIFIED REJECT** (rc=1): `error: Declared instance key (A, B) on
local 'rel' has no matching demanded query adornment; fix or remove the @key pragma`
(V-DECLARED-KEY Arm A, Demand.cpp:929-946). This is the category error the rewrite
fixes made concrete: `@key(A,B)` "has no matching query adornment" *because @key is not
a query adornment* — the demand bijection wrongly demands one. **No baseline; pure target.**

**Post-P5 — PREDICT:** compiles; `@key(A)` and `@key(A,B)` SHARE one `{A}` schema:

```
# -region-out binding-state block (P5)
  binding-schema  S0  rel  fields={}          # empty/root
  binding-schema  S1  rel  fields={A}         # ONE {A} schema — shared, NOT two
  binding-schema  S2  rel  fields={A,B}
  binding-edge    empty --A--> S1             # ONE empty->{A} edge (both paths' first hop)
  binding-edge    S1    --B--> S2             # only @key(A,B) adds this hop
# .contract (F21 canonicalized, KeyPathId order):
  declared-key rel=rel declared=(A)
  declared-key rel=rel declared=(A, B)        # intra-path order KEPT; inter-path order-free
```

Exit gate (reconstruction-diffs §3 P5, F8): exactly ONE `binding-schema fields={A}` and
ONE `empty--A-->{A}` edge (prefix sharing — NOT two); the declared prefix chain
`{A}⊂{A,B}` present. (A `@key(A,B,C)` sibling additionally pins: NON-prefix subsets
`{A,C},{B},{C}` ABSENT — the F8 correction; the s12 "no {A,B}" clause was wrong.)

### 7B. Order-convergence — `@key(A,B) @key(B,A)` (roadmap test 9)

```datalog
#message edge_2(u64 A, u64 B).
#local rel(u64 A, u64 B) @key(A, B) @key(B, A).
rel(A, B) : edge_2(A, B).
#query qab(bound u64 A, bound u64 B, free u64 X) : rel(A, B), edge_2(A, X).
```

**Current @ tip — VERIFIED REJECT AT PARSE** (rc=1): `error: Duplicate instance key on
local 'rel'; this '@key' pragma declares the same column set as an earlier '@key'
pragma` (ADJ-K1-A, Parser.cpp:974-995 — the same-decl dup check that SORTS each set, so
`[A,B]` and `[B,A]` collide). **This reject is UPSTREAM of the demand pass**, so it
survives P1 untouched — **7B requires Phase-0 item 4 (delete the intra-key sort at BOTH
Parser.cpp:974-995 same-decl and Parser.cpp:1477-1488 cross-redecl) before P5 can compile
it.** Empirical proof that Phase-0 item 4 is a hard prerequisite for the P5 convergence
witness, not optional polish. **No baseline; pure target.**

**Post-(P0-item-4 + P5) — PREDICT:** compiles; `[A,B]` and `[B,A]` are DISTINCT ordered
paths that CONVERGE on one `{A,B}` schema:

```
# -region-out binding-state block (P5)
  binding-schema  S0  rel  fields={}
  binding-schema  S1  rel  fields={A}
  binding-schema  S2  rel  fields={B}
  binding-schema  S3  rel  fields={A,B}       # ONE {A,B} schema (order-free convergence)
  binding-edge    empty --A--> S1
  binding-edge    empty --B--> S2
  binding-edge    S1    --B--> S3             # [A,B] chain
  binding-edge    S2    --A--> S3             # [B,A] chain — TWO edges, ONE endpoint
# .contract (F21): declared-key rel=rel declared=(A, B)  +  declared=(B, A)  (both legal, distinct)
```

Exit gate (P5, goal 2): TWO distinct ordered `binding-edge` chains terminating at ONE
`{A,B}` `binding-schema` id; both `declared=(A, B)` and `declared=(B, A)` present and
distinct in `.contract`; reordering the two pragmas in source → byte-identical
`-contract-out` (F21). Retracting one path's derivation preserves a fact still supported
by the other (roadmap test 15) — a `.batches` witness.

---

## §8. Addendum to the self-critique (session 13)

7. **The three "no baseline" carriers now have VERIFIED CURRENT REJECTS** (§6.0/§7A/§7B,
   compiled read-only this session), which strengthens the predict-then-verify contract:
   the verify step is not just "byte-diff a new golden" but "the program that REJECTS at
   tip must COMPILE and produce the pinned structural shape after its phase" — a
   reject→compile transition is a stronger witness than a golden re-bless.
8. **7B empirically pins a phase dependency the diffs assert:** `@key(A,B) @key(B,A)`
   rejects AT PARSE (ADJ-K1-A), upstream of demand, so it survives P1 and blocks the P5
   convergence witness until Phase-0 item 4 lands. This is the entangled-with-P1 item the
   seed §4 P0 flags; 7B is its concrete test. (The Parser.cpp:974-995 same-decl dup check
   and Parser.cpp:1477-1488 cross-redecl check are TWO sort sites; item 4 must fix both.)
9. **P4's C++ desired state (§2.4) is REVISED by the session-13 grounding:** the honest
   FullScanFilter emission REUSES `ProgramTableScanRegion` (Program.h:1096-1140) with
   `index=nullopt`, so the desired generated C++ is the EXISTING `EmitScan` full-scan mold
   (Database.cpp:3334-3396) with a key-equality filter over `InputVariables` — NOT a new
   `emit_instance_rescan`-shaped region. Label==emission holds by construction (no separate
   DR label to drift). The §2.4 structural pin becomes: grep generated `datalog.h` for the
   `for (uint32_t s… < <table>.NumRows(); ++s) { … if (r.<key0> == <bound0> …) }` loop
   tied to the ACTUAL bound var, NOT for any new instance-region marker. **CONFIRMED** by the
   session-13 P4 refuter: `EmitScan` (Database.cpp:3374-3418) with `index=nullopt` + non-empty
   `InputVariables` falls to the full-scan arm then emits the key filter at :3403-3418
   (`assert(indexed_cols.size()==input_vars.size())`); public handle `ProgramTableScanRegion`
   Program.h:1097-1140, Impl at lib/ControlFlow/Program.h:1601. **CAVEAT (reconstruction-critique
   B3/P4):** the desired C++ probe must DISCRIMINATE against the P1 baseline — the post-P1
   bound-query cursor at Database.cpp:1768-1799 ALREADY emits a `NumRows` scan + key filter
   (`while (pos < db.<member>.NumRows()) { … if (row.<field> != <param>) continue; }`), so the
   pin keys on the region-cursor `s<id>` shape (vs the query-cursor `pos`) AND on a
   `-region-out` compile-time RequestEdge/RoutedResult assertion (RoutedResult has no datalog.h
   surface at P4). The P4 emission hunk must also populate `out_vars` (one VAR per table column,
   per Join.cpp:268-270) or the scan body's free-column refs are unbound.

---

## §9. P7 desired IR states — the AccessPlan render + honest emission (carrier: key_neighborhood_witness)

Session 14. The P7 desired states, WITH the session-14 critique corrections folded
(`keyed-rewrite-p7p9-critique.md`; the diffs are `keyed-rewrite-p7p9-diffs.md` §1 + §7). All
baselines VERIFIED read-only this session; post-cut states are PREDICTIONS.

### 9.0 The two REAL baselines (verified in the generated header this session)

`key_neighborhood_witness` compiled read-only; `neighborhood` carries an index on `From` (`idx_41`).
The generated `datalog.h` shows the TWO shapes the P7 pin must discriminate:

```cpp
// (a) P1-BASELINE #query cursor — a <name>_cursor STRUCT keyed on pos/id (Database.cpp:1735-1829):
struct neighborhood_bf_cursor { ...
    pos = db.idx_41.Next(id);                     // ALREADY .First/.Next — the relation has an index!
    const auto row = db.neighborhood_4.RowAt(id); ... };
friend neighborhood_bf_cursor neighborhood_bf(Database &db, ..., uint64_t Start) {
    return {db, Start, db.idx_41.First({Start})}; }
// (b) region rescan — a REGION cursor keyed on s<id> (EmitScan, Database.cpp:3334):
for (uint32_t s = 0; s < table_11.NumRows(); ++s) { const auto ir = table_11.RowAt(s); ... }
```

**CRITICAL (critique M-P7-loopshape):** because `neighborhood` HAS an index, baseline (a) already
emits `idx_41.First/.Next`. So the loop-shape pin (`First/Next` vs `NumRows`) does NOT discriminate a
no-op P7 from a real one on the QUERY path. **The load-bearing P7 discriminator is the CURSOR SHAPE:**
the baseline is a `neighborhood_bf_cursor` struct keyed on `pos`/`id`; a P7 region read is `s<id>`
inside a region (certification 1). The loop-shape pin (§1.6.1) survives only for the interior/rule-body
path where no query cursor exists.

### 9.1 Post-P7 `.rel` — the AccessPlan render (PREDICT)

```diff
# .rel — the keyed read op gains an AccessPlan attribute (the physical-structure authority):
+op.K kAccess(%table:edge) plan=FullScanFilter  bound=[From@0]  cursor=region  -> kFold(neighborhood,+,NonRecursive)
#      ^ plan= is the AccessPlan kind; NOT the join Lowering badge (which stays section-walk/full-scan,
#        advisory, on JOIN ops only — §1.1 keeps the two domains DISJOINT). A keyed complete-path read
#        with a strict-subset bound would render plan=FullKeyHashLookup; an all-columns-bound read
#        renders plan=FullKeyExactProbe (⊗ s15: its OWN kind, but index=SOME — the codegen-DEAD all-col
#        index that routes control to keyed_probe's member.Find; NOT index=nullopt, which would fall to
#        the full-scan arm, Database.cpp:3379).
```
STRUCTURAL pin: the `.rel` census gains an `kAccess`-family count; `plan=` is an exact token per op.
A no-op P7 (everything `plan=FullScanFilter`) is distinguishable from a real one (a strict-subset bound
renders `plan=FullKeyHashLookup`) — a census/token pin, not answer-equality.

### 9.2 Post-P7 generated C++ + the emission-fidelity belt (PREDICT)

```cpp
// FullScanFilter (index=nullopt) — the honest arm, region cursor s<id>:
for (uint32_t s0 = 0; s0 < db.edge_11.NumRows(); ++s0) {
  const auto r0 = db.edge_11.RowAt(s0);
  if (r0.col0 == Start) { const auto To = r0.col1; /* body */ } }
// FullKeyHashLookup (index=Some, strict-subset bound) — keyed_chain, region cursor s<id>:
for (uint32_t s0 = db.idx_NN.First({Start}); s0 != ::hyde::rt::kNoRow; s0 = db.idx_NN.Next(s0)) {
  const auto r0 = db.edge_11.RowAt(s0); const auto To = r0.col1; /* body */ }
```
The V-PLAN-HONEST belt is an EMISSION-SITE assert (⊗ s15: a `kUnplanned` sentinel default so the belt
SKIPS the TWO pre-existing mint sites — join pivots Join.cpp:254 AND the Build.h:469 crossover scans, one
of whose arms is index=None — that never set `plan_kind`; per-kind ARM implications, NOT an
`Index()==nullopt` biconditional since kFullKeyExactProbe has index=Some yet is not a hash lookup):
```
EmitScan(region):  # Database.cpp:3334 head
    if region.plan_kind != kUnplanned:                                   # skip join/crossover scans
        assert( region.plan_kind == kFullScanFilter    => arm is full_scan_filter )
        assert( region.plan_kind == kFullKeyExactProbe => arm is keyed_probe )    # member.Find (index=Some, dead)
        assert( region.plan_kind == kFullKeyHashLookup => arm is keyed_chain )    # First/Next over a real idx
        assert( region.plan_kind == kTriePrefixWalk    => arm is TrieRange )      # P8; dead until caps admit
```
STRUCTURAL pins: (1) CURSOR-SHAPE — `s<id>` region cursor, NOT a `<name>_bf_cursor` struct pos/id
(the load-bearing query-path discriminator, §9.0); (2) the `plan_kind⇔Index()` bijection assert
(aborts at compile on a drift); (3) `in_vars` ascending-sorted to match `KeyColumns` order
(critique H-P7-inkey-order — a mis-ordered `[B,A]` key silently returns empty; the pin is that the
emitted `First({...})` arg order matches the ascending Key struct layout).

### 9.3 P7 phases 8–9 for this carrier: NO CHANGE (single key on `From`; no trie, no inference beyond it).

---

## §10. P8 desired IR states — the trie block (carrier: key_multi_adorn_witness + a NEW convergence probe)

P8 physically realizes the P5 partial-binding DAG. The desired observation surface REUSES the P5
`-region-out` binding-schema block (critique M-P8-nodecount fix: a runtime node count is NOT
golden-able; pin the COMPILE-TIME eager schema spine, which §2.4's one-to-one TrieNode↔schema makes
identical to the P5 surface).

### 10.1 Post-P8 `-region-out` — the binding-schema spine + trie realization (PREDICT)

For a relation with `@key(A,B) @key(B,A)` (the §7B convergence probe, post-P0-item-4 + P5 + P8):
```
# -region-out (the P5 binding-schema block IS the P8 trie's compile-time spine — one surface):
  binding-schema  S0  rel  fields={}                    origin=declared
  binding-schema  S1  rel  fields={A}                   origin=declared
  binding-schema  S2  rel  fields={B}                   origin=declared
  binding-schema  S3  rel  fields={A,B}                 origin=declared   # ONE {A,B} schema (convergence)
  binding-edge    S0 --A--> S1                          # [A,B] chain hop 1
  binding-edge    S1 --B--> S3                          # [A,B] chain hop 2
  binding-edge    S0 --B--> S2                          # [B,A] chain hop 1
  binding-edge    S2 --A--> S3                          # [B,A] chain hop 2 -> SAME S3 (two edges, one node)
  trie-index      T0  rel  path=[A,B]  backing=S0->S1->S3   # the ORDERED physical realization (kind=kTriePrefix)
  trie-index      T1  rel  path=[B,A]  backing=S0->S2->S3   # shares S3's terminal (convergent endpoint)
```
STRUCTURAL pins (critique M-P8-nodecount — all COMPILE-TIME, over the eager MaterializePrefixChain
spine, `origin=declared`, before any EvaluateEpoch): (1) EXACTLY ONE `binding-schema fields={A,B}`
(S3) reachable by TWO `binding-edge`s — the convergence pin (nodes = {A},{B},{A,B} = 3, never 4);
(2) a runtime `origin=visited` node is NOT counted here (it renders only if a probe minted it — the
declared-vs-visited origin tag, folded from reconstruction-diffs §5.6 M7). For prefix-sharing
`@key(A) @key(A,B)` (§7A): ONE `binding-schema fields={A}` shared, ONE `S0--A-->S1` edge, the
`{A}⊂{A,B}` chain present, non-prefix subsets absent.

### 10.2 The runtime trie node identity (critique B-P8 fix — VERIFY at implementation)

The runtime trie node is interned on `BindingStateId = (schema, sorted value-map)`, NOT the value-free
schema alone (F11 is the P5 COMPILE-lattice rule; the P8 RUNTIME node is a BindingStateId). This makes
a node BOTH value-partitioned (so `TrieFirst(full_key).head` returns ONE value's row chain, honoring
the full-key-EXACT no-recheck contract, Table.h:791-803) AND convergent (two value maps that sort-equal
share ONE node). NO golden observes runtime nodes — the pin is the compile-time spine (§10.1).

### 10.3 Post-P8 generated C++ — the ORDERED `.Range` (the ONE new codegen surface, PREDICT)

**⊗ s15 SCOPE:** the intra-relation prefix seek is NOT here — it is P7 `kFullKeyHashLookup` (§9.2's
`idx_NN.First({...})/Next` over a bound-subset hash index; `plan=FullKeyHashLookup`). The genuinely-new
P8 surface is the ORDERED range — seek to a prefix node and enumerate its ordered subtree (all extensions
IN key order), which a hash `First/Next` (one exact-key chain) cannot do — needed for the cross-relation
Free Join lock-step descent:

```cpp
// TrieRange(bound_prefix=[A], values={A=a}) — seek to the ordered prefix node, DFS-enumerate the subtree:
for (uint32_t s0 = db.trie_T0.RangeFirst({a}); s0 != ::hyde::rt::kNoRow; s0 = db.trie_T0.RangeNext(s0)) {
  const auto r0 = db.rel_NN.RowAt(s0); /* liveness via DiffTable::Present(r0) */ ... }
```
This is the genuinely-new `EmitAccessPlan` branch over an ORDERED `kTriePrefix` index (§2.2/§2.3). STRUCTURAL
pin: `RangeFirst/RangeNext` present ⇔ `plan=ExistingTriePrefix` in `.rel` ⇔ `kTriePrefixWalk ∈
CodegenPlanCapabilities` (the P8→P7 caps handshake). Until the ordered trie runtime + `.Range` land,
`kTriePrefixWalk` stays OUT of caps and this shape NEVER emits — a bound-prefix intra-relation read is
served by P7 `plan=FullKeyHashLookup` (correct, just unordered).

### 10.4 F20 completeness pin: `TrieRange(root)` enumerates == full DiffTable scan (an UNBOUND read
returns the COMPLETE answer, never an active-prefix subset — the ActiveSubset-trap guard). A count-equality
assert after any keyed materialization, not answer-equality.

---

## §11. P9 desired IR states — the inferred-path contract render (RE-TARGETED to Regional)

P9 adds inferred logical paths additively; the declared path is authoritative and always survives.

### 11.1 The render SEAM MOVES DataFlow→Regional (critique H-P9-render-anchor)

The current single-line `-contract-out` renderer (DataFlow/Format.cpp:1745-1798) reads the P1-DELETED
`RecognizedSubgraphs` and is a DataFlow dump that cannot see where P9 stores inferred paths. **⊗ s15
DATA PATH:** P9 (`InferAccessPaths`) runs PRE-Optimize and deposits `impl->inferred_access_paths` — a
`QueryImpl` MEMBER keyed by Optimize-stable parse identity `(relation_decl_id, PathSourceKey)` (the
`row_contracts` precedent, NOT the destroyed-at-return `proxy_view_to_decl` — critique
`satellite-lifetime-contradiction`). P2/Regional folds that member into `schema.access_paths`; the
declared/inferred render is a REGIONAL/typed dump sourced from `schema.access_paths` (provenance-tagged),
composing with P2's "formatting derives from typed records." `-contract-out` becomes a Regional surface.

### 11.2 Post-P9 render — SPLIT declared/inferred provenance lines (PREDICT)

Baseline (tip): `declared-key rel=edge declared=(From) inferred=(From)` — ONE line, the SIP bijection
shape (`inferred=` is the `p_bound` the declared set had to EQUAL, Format.cpp:1757, certification 13).
Post-P1: `declared-key rel=edge declared=(From)` (the `inferred=` half unreconstructable, @key inert).
Post-P9:
```
declared-key rel=edge declared=(From)                       # provenance=kDeclared — ALWAYS present
inferred-key rel=edge inferred=(From, To)  source=join      # provenance=kInferred — additive, 0+ lines
inferred-key rel=edge inferred=(From)      source=query     # a query adornment as ONE inferred source
```
STRUCTURAL pins (critique-verified — all provenance-survival, never answer-equality):
- (1) ANTI-REGRESSION (the belt the deleted bijection would have failed): a declared path with NO
  matching inference SURVIVES verbatim as one `declared-key` line. This is what V-DECLARED-KEY
  (Demand.cpp:892-960) wrongly REJECTED; P9 keeps it (V-DECLARED-PATH-PRESERVED, certification 10).
- (2) ADDITIVE-NOT-MERGE: `[A,B]` declared + `[B,A]` inferred ⇒ TWO lines, TWO KeyPathIds — never
  merged (a merging P9 collapses them; the answer is identical, so only a structural pin catches it).
- (3) F21 ORDER-FREE: reordering two `@key` pragmas ⇒ byte-identical render (KeyPathId-sorted).
- (4) SOURCE ISOLATION: an `inferred-key`/`declared-key` line carries LOGICAL field names only, NEVER
  an AccessPlan/PhysicalAccessStructure token (those live in `.rel`/codegen — a physical token in the
  contract render is a layer-violation finding). (certification 15)
- (5) SOURCE-3 back-reference (critique L-P9-split-source3): a CONTEXTUAL inferred path renders a
  `base=<KeyPathId>` token so the base→derived pairing SOURCE 3 generates is not lost (else the split
  is not a strict information superset).

### 11.3 Goldens that MOVE (critique certification 12): exactly `key_multi_adorn_witness.contract`
(2 declared-key lines) and `key_tc_witness.contract` (1 line) — both REAL files, not symlinks; no other
`.contract` golden is affected. The render-seam move (§11.1) + the split (§11.2) re-bless both. This is
an owner-adjudicated golden-shape change (flagged p7p9-diffs §6.4).

---

## §12. Session-14 + Session-15 addenda to the self-critique

10. **The P7–P9 desired states import the physical-layer "structural-pins-only" discipline** (§5's
    lesson, re-confirmed by the critique): EVERY P7–P9 pin is a cursor-shape / census-token /
    compile-time-node-count / provenance-survival pin — never answer-equality, because the P1
    full-materialization baseline AND the honest FullScanFilter both answer correctly, so a no-op
    P7/P8/P9 passes any answer test.
11. **Two desired-state anchors are now CORRECTED by the critique, not merely predicted:** (a) the P7
    query-path discriminator is CURSOR-SHAPE, not loop-shape (§9.0, verified in the real generated
    header — `neighborhood`'s index makes the baseline already emit `First/Next`); (b) the P9 contract
    render MOVES to a Regional dump (§11.1, because the DataFlow renderer reads P1-deleted symbols).
    Both are stronger than a golden re-bless — they are seam/shape corrections grounded in opened code.
12. **The P8 observation surface is the P5 `-region-out` binding-schema block, not a new trie dump**
    (§10.1) — the one-to-one TrieNode↔BindingStateSchema (§2.4) means the compile-time spine is already
    rendered by P5; P8 adds only `trie-index` lines pointing at that spine. A runtime node count is
    deliberately NOT goldened (mint-on-visit is data-dependent).

### Session-15 corrections (the amended diffs + their re-critique, folded)

13. **P7 `kFullKeyExactProbe` renders `index=SOME`, not `index=nullopt`** (§9.1/§9.2). Verified this
    session (Database.cpp:3379): `keyed_probe` requires `region.Index()` truthy, so the exact-probe kind
    carries the codegen-DEAD all-column index that routes control to `member.Find`; `index=nullopt` would
    fall to the full-scan arm. The `.rel` `plan=FullKeyExactProbe` token still discriminates (its own
    kind), but the C++ pin is `member.Find` + the belt's per-kind ARM implication, not an `index==nullopt`
    biconditional.
14. **The intra-relation prefix seek is P7, not a P8 trie** (§10.3, s15 critique). It renders
    `plan=FullKeyHashLookup` and emits `idx.First/Next` over a bound-subset hash index — the s15-first-draft
    "P8a prefix sibling index" was redundant with P7 (it broke the plan_kind CSE-safety proof, was an
    unreachable arm, and duplicated the index). The genuinely-new P8 `.rel`/C++ surface is the ORDERED
    `.Range` (`plan=ExistingTriePrefix`, `RangeFirst/RangeNext` DFS over an ordered `kTriePrefix` index),
    which a hash `First/Next` cannot express.
15. **The P9 render sources from a `QueryImpl` MEMBER** (§11.1) `impl->inferred_access_paths` keyed by
    parse identity `(relation_decl_id, PathSourceKey)` — NOT a Build-scoped `proxy_view_to_decl` (destroyed
    at return). The `source=query/join/contextual` token on each `inferred-key` line is the `PathSourceKey`
    that also keys the union (so SOURCE 2/3 non-adornment paths are not dropped). Both grounded in opened
    code + the s15 predict-then-verify (the `demand_tc_witness` `.df` TUPLE/MERGE collapse forcing
    pre-Optimize inference).
