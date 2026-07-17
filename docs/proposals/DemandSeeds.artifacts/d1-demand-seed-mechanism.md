# D1 — THE DEMAND-SEED MECHANISM: compiler-synthesized demand messages, per demanded adornment

Lane D1, DESIGNER, demand-seeds / keyed-instances epoch. Written
2026-07-17 against branch `demand-seeds` (tip 84bb39f1 = main ca569dd8 +
the D0 ledger commit). This is the epoch's CHARTER-DIFF design: the
generalization of the forcing-message surface (`force.dr`,
`ParsedClause::ForcingMessage`) into compiler-SYNTHESIZED demand messages
per demanded adornment `p^α`. R4/P3 discipline applies — NO demand code
is written until this design survives its adversarial judge, so every
claim below is falsifiable and anchored to a file:line read THIS session
on THIS branch.

Binding inputs honored BY NAME: D0 errata E-37 (the THREE forcing gates),
E-45 (the `messsage_handler` wiring dependency), E-41 (tc = FOUR
adornments {bf,fb,ff,bb}), E-40 (38/165 bound-query cases), E-44 (Log
threaded-unused on the inject path); the p3-demand-argument.md §2 (a)-(f)
obligations (the standing gate); the §1.2 ⊥c-pivot demand-edge shape; the
§3.1 `demand_tc_witness.dr`; E-32 (structural demand edge, never
group_ids); the SLDMagic memory (a transformation, NEVER an evaluator).

--------------------------------------------------------------------------
## 0. ANCHOR TABLE (every code claim, file:line, read this session)

| # | Anchor | Fact |
|---|--------|------|
| A1 | Build.cpp (CF) :246-368 | `BuildQueryForceProcedureImpl`: builds `kQueryMessageInjector` proc; body VECTORAPPEND(bound→add_vec) → CALL `context.messsage_handler[message]` (:356) → RETURN true (:363-365) |
| A2 | Build.cpp (CF) :373-381 | `BuildQueryForceProcedure`: gated on `query.ForcingMessage()` (:375); else nullopt |
| A3 | Build.cpp (CF) :384-412 | `BuildQueryEntryPointImpl`: collects kBound col_indices (:393-398); GetOrCreateIndex iff non-empty (:405-409); forcer_proc from A2 (:401-402); `impl->queries.emplace_back(query, table, scanned_index, forcer_proc)` (:411) |
| A4 | Build.cpp (CF) :356 | injector CALL target = `context.messsage_handler[message]` — the E-45 lookup |
| A5 | Procedure.cpp :383-399 | `BuildIOProcedure`: returns early if `io.Receives().empty()` (:386-388); ELSE populates `context.messsage_handler.emplace(message, io_proc)` (:399) |
| A6 | Build.cpp (CF) :1233-1235 | IO procs built (populating messsage_handler) for every `query.IOs()` |
| A7 | Build.cpp (CF) :1241-1248 | query entry points built AFTER (A6), over `query.Inserts()` where `insert.IsRelation() && decl.IsQuery()` |
| A8 | Parse.cpp :1021-1028 | `ParsedClause::ForcingMessage`: `assert(forcing_predicates.Size()==1u)` (:1025); zero→nullopt (:1022-1023) — AT-MOST-one per clause (E-37 gate 1) |
| A9 | Parse.cpp :1142-1155 | `ParsedQuery::ForcingMessage`: `assert(!pred)` (:1146) across ALL clauses — AT-MOST-one per query (E-37 gate 2) |
| A10 | DataFlow Build.cpp :1911-1923 | forced query must have `UniqueRedeclarations().size()==1u` else clean diagnostic (E-37 gate 3) |
| A11 | DataFlow Build.cpp :1658-1677 | `BuildClause`: a forcing pred brings its `forced_view = BuildPredicate(...)` (:1669) in FIRST, pushed to `view_groups[0]` (:1671) |
| A12 | DataFlow Build.cpp :1901-1909 | the forced-view param check asserts message_decl.IsMessage / query_decl.IsQuery — the forced view is a real DataFlow message receive |
| A13 | force.datalog.h :118-123 | `get_next_id_bf(Database&, Log&, Functors&, int64_t Time)` (Log unnamed/unused — E-44); body calls `inject_20(...)` then returns a cursor over `idx_24.First({Time})` |
| A14 | force.datalog.h :159-167 | `inject_20`: builds `Vec<Tup_i64>`, `.Add({v21})`, calls `trigger_generate_next_id_1_detail(...)` — the message detail proc |
| A15 | force.ir :32-42 | `^receive:trigger.../1` and `^inject:20` — the inject calls the receive proc; both real IR procs |
| A16 | p3 §1.2 / Build.cpp (DF) :1450-1512 | `ApplyPositiveConditionTest` ⊥c 1-pivot presence-join — the demand edge's structural precedent (E-32) |
| A17 | View.cpp :1478-1480 | `InsertSetsOverlap` returns MERGEABLE on empty group_ids (the E-32 unsoundness root) |
| A18 | Optimize.cpp :410-434 | group_ids seed ONLY on JOIN/AGG/KVINDEX; everything else empty |
| A19 | Join.cpp :467 | JOIN `Equals()` short-circuits on num_pivots/children (structural distinctness before group_ids) |
| A20 | Stratify.cpp :387-419 | the always-on induction cross-check (inductive==same_scc, stratum order, merge_set==SCC partition) |
| A21 | DataFlow Build.cpp :2550-2610 | `Query::Build` pipeline order; the demand pass slots at :2566, before Optimize (p3 §1.3) |
| A22 | transitive_closure.dr :1-16 | 3 #query decls (reachable_from=bf, reaching_to=fb, is_node=f) projecting from `#local tc`; recursive rule tc(F,T):-tc(F,X),tc(X,To) |

--------------------------------------------------------------------------
## 1. THE MECHANISM

### 1.0 The three objects, and the ONE structural insight

A demanded adornment `p^α` (predicate `p`, adornment string `α` marking
which columns are bound) needs THREE synthesized objects. Naming them
precisely, because the pipeline level of each is the entire design
question:

  1. **the demand relation `d_p^α`** — a fresh relation whose rows are the
     ground tuples of `p`'s BOUND-column positions (under `α`) for which
     `p` is actually demanded. Arity = |bound cols of α|.
  2. **the demand-guarded copy `p'^α`** — a structural copy of `p`'s
     rule bodies joined against `d_p^α` on the bound columns (the ⊥c-pivot
     shape, A16/E-32). Produces only rows whose bound-projection is in
     `d_p^α`.
  3. **the root seed + forcer** — the runtime bound argument (supplied at
     query-call time) must land in `d_p^α`. This is the object the
     forcing-message surface already provides for `force.dr`: an
     injector proc that batches the bound tuple into a message handler.

THE STRUCTURAL INSIGHT that decides §1's level question: the
forcing-message mechanism is NOT a single object — it is a THREE-LEVEL
stack, and the three levels are wired by two separate lookups:

  - PARSE level: `force.dr` writes `@first trigger_generate_next_id(Time)`
    inside the query clause. `ParsedClause::forcing_predicates` (A8) and
    the message decl exist as parse objects.
  - DATAFLOW level: `BuildClause` sees the forcing pred and mints
    `forced_view = BuildPredicate(...)` (A11) — a REAL DataFlow message
    RECEIVE view, pushed into the clause's view group. Because the
    message is `#message`-declared, DataFlow also builds a `QueryIO` with
    a non-empty `Receives()`.
  - CONTROLFLOW level: `BuildIOProcedure` (A5) sees that `QueryIO` has a
    non-empty receive and populates `context.messsage_handler[message]`
    (:399). ONLY THEN does `BuildQueryForceProcedureImpl` (A1) find a
    handler to CALL at :356. The injector→handler edge is a
    `messsage_handler` map lookup keyed by `ParsedMessage`.

E-45 is exactly this: the forcer's CALL target is populated ONLY by a
real QueryIO-with-receive, built at A6 STRICTLY BEFORE the entry points
at A7. A synthesized demand message that has no parsed `#message` + no
DataFlow receive has NO `messsage_handler` entry and NO `forced_view`
flow. So the design question is: **at which of these three levels does
the demand transform synthesize its objects, so that all three lower
correctly?**

### 1.1 The demand relation `d_p^α` and its guarded copy `p'^α` (level-independent core)

Regardless of the message-level decision below, the demand RELATION and
the GUARDED COPY are DataFlow-graph objects, minted by a dedicated pass
inside `Query::Build`, slotted at A21:2566 (AFTER
`ConnectInsertsToSelects`, BEFORE `Optimize`, BEFORE `IdentifyInductions`/
`Stratify`) — the ordering p3 §1.3 argues and this design ratifies (§3).

`d_p^α`: an ordinary relation, arity |bound(α)|, whose producing sources
are (i) the ROOT SEED (the runtime bound tuple — §1.2) and (ii) the
DEMANDING SUBGOALS (recursive demand propagation — p3 §1/(f)). It lowers
as an ordinary table (differential or monotone, DERIVED by
`TrackDifferentialUpdates` on the rewritten graph — never asserted by the
transform).

`p'^α = p ⋈ d_p^α` on the |bound(α)| pivots, the A16 ⊥c-pivot shape
generalized from 1 presence-pivot to N key-pivots:

```
      p-body [x̄, ȳ]            SELECT d_p^α [x̄]      <- demand relation SELECT
              \                    /
             JOIN (pivots: x̄) [x̄, ȳ]                 <- |x̄| ≥ 1 real column edges
                    |
             TUPLE [x̄, ȳ]                             <- canonical column identity
```

Structural distinctness (§3(b)): `p'^α`'s top view is a JOIN with
`num_pivots = |x̄| ≥ 1` and an extra `d_p^α`-SELECT child; the unguarded
twin `p` has neither. `Equals()` short-circuits on num_pivots/children
(A19) BEFORE `group_ids`/`InsertSetsOverlap` (A17/A18) is consulted. This
is the whole of E-32's satisfaction — and it is level-INDEPENDENT (holds
no matter how the message is synthesized). So §1's contested decision is
ONLY about object 3 (the root seed + forcer): **at what representation
level does the demand MESSAGE exist**, so that the runtime bound argument
reaches `d_p^α`'s producing edge.

### 1.2 The contested decision: at what level does the demand message exist?

The runtime bound argument is delivered — like `force.dr` — by an
INJECTOR proc that batches the bound tuple and CALLs a message handler.
For `force.dr` that handler is the real `#message`'s receive proc (A5).
For a SYNTHESIZED demand, there is no user `#message`. Three candidate
levels, each priced against the six pressures the D0 fleet named: the
three E-37 gates, the E-45 wiring, the debug parser round-trip (CLAUDE.md
gotcha: debug builds re-parse and re-assert — a fabricated parse object
must survive that), id-stream / golden stability, and the mode-gate.

#### Option P — Parse-level fabrication (fabricate a `ParsedMessage` + `@first` forcing pred per adornment)

Synthesize, at parse-post-processing, a fresh `#message d_p^α(bound-cols)`
declaration and inject a `@first d_p^α(...)` forcing predicate into a
demand-guard clause, exactly as `force.dr`'s author hand-writes. Then the
ENTIRE existing stack (A11→A5→A1) fires unchanged — this is the literal
reading of §16.5(B) "synthesize what force.dr makes the user write."

  - E-37 gate 1 (A8 clause `assert(Size()==1u)`): a per-adornment scheme
    puts ONE synthesized `@first d_p^α` per demand-guard clause — SATISFIED
    if each adornment gets its OWN clause. BUT the ORIGINAL query clause
    (e.g. `reachable_from(F,T):tc(F,T)`) is untouched and has zero forcers;
    the synthesized guard clauses each carry exactly one. OK per clause.
  - E-37 gate 2 (A9 query `assert(!pred)` across ALL clauses of a query):
    THIS IS THE KILLER. `ParsedQuery::ForcingMessage` walks EVERY clause of
    the query and asserts at most one forcing pred TOTAL. A query demanded
    at multiple adornments (tc: {bf,fb,ff,bb}, E-41) would need multiple
    forcing preds across its clauses → the assert (:1146) ABORTS in debug.
    To use Parse-level fabrication we would have to WEAKEN or DELETE A9 —
    a change to the parser's forcing contract that ripples into every
    `ParsedQuery::ForcingMessage` caller (A1's `BuildQueryForceProcedure`
    at :375 assumes at-most-one). High blast radius.
  - E-37 gate 3 (A10 `UniqueRedeclarations().size()==1u`): a demanded
    query MAY legitimately have multiple redeclarations (bf and fb ARE two
    binding patterns of the same relation in tc). This gate rejects them
    TODAY. Parse-level fabrication inherits this reject — we'd have to
    relax it too.
  - E-45 wiring: SATISFIED for free — a fabricated `#message` produces a
    real `QueryIO` with a receive, so A5 populates `messsage_handler` and
    A1's CALL target exists. This is Option P's ONE real advantage.
  - DEBUG PARSER ROUND-TRIP: THE SECOND KILLER. Debug builds re-parse and
    re-assert (CLAUDE.md gotcha). A fabricated `ParsedMessage` /
    `ParsedPredicate` that was never lexed from source has no display
    tokens, no `DisplayRange`, no spelling. The round-trip re-parse (and
    every `SpellingRange()`-consuming diagnostic, e.g. A10:1913) would hit
    invalid tokens → assert/abort. Fabricating parse objects that survive
    the round-trip means synthesizing a full token stream — effectively
    generating `.dr` source text and re-lexing it. That is a large,
    fragile surface (the SLDMagic "never an evaluator" spirit says: do not
    build a source-generator either).
  - id-stream / golden stability: fabricating parse objects perturbs
    `IdInClause`/decl-id minting for EVERY downstream case even when the
    pass is a "no-op" unless carefully fenced — a mode-gate that only
    skips the DataFlow pass would still have paid the parse-object cost if
    fabrication happens at parse time. Fragile to keep byte-identical-off.
  - VERDICT: REJECTED. Three gate-weakenings (A8/A9/A10) + a parse-object
    round-trip hazard. Its only win (free E-45 wiring) is obtainable more
    cheaply at the DataFlow level.

#### Option D — DataFlow-level minting (mint the QueryIO + receive + forced_view directly, no ParsedMessage)

Mint, in the demand pass at A21:2566, the demand relation `d_p^α` AND a
synthetic `QueryIO` carrying a non-empty `Receives()` for it, plus the
`forced_view` message-receive view — the objects A11/A12 build from a
parsed forcing pred, but minted directly from the transform without a
`ParsedMessage`.

  - E-37 gates 1-3 (A8/A9/A10): ALL THREE ARE PARSE/DATAFLOW-VALIDATION
    gates on the USER-WRITTEN forcing surface. If the demand pass mints
    the receive/forced_view DIRECTLY, it NEVER calls
    `ParsedClause::ForcingMessage` / `ParsedQuery::ForcingMessage`, so A8
    and A9 are never reached for the synthesized objects. A10 fires only
    in the `if (forced_view)` block of the USER path (A11-A12); a
    directly-minted demand view is not a user forced_view and skips it.
    ALL THREE GATES BYPASSED WITHOUT WEAKENING THEM — the user forcing
    surface keeps its exact contract; the demand path is a parallel,
    unguarded-by-those-asserts construction. This is the decisive win.
  - E-45 wiring: the demand pass must ALSO ensure the ControlFlow
    `messsage_handler` gets an entry for `d_p^α`'s injector to CALL. Since
    we minted a real `QueryIO` with a non-empty receive, A5's
    `BuildIOProcedure` populates `messsage_handler` at :399 in the SAME
    A6 loop that handles user IOs — the demand message is just another IO.
    SATISFIED, and by the SAME code path the E-45 fact identifies, no
    bypass needed.
  - DEBUG PARSER ROUND-TRIP: the demand objects live BELOW the parser —
    they are `QueryImpl`/`QueryIO`/`QueryView` nodes, minted after
    `BuildClause`. The debug round-trip re-parses the ORIGINAL `.dr`
    SOURCE (which is unchanged) and re-runs `Query::Build` (which re-runs
    the demand pass deterministically). There is no fabricated parse
    object to survive a re-lex — the transform is a graph rewrite that
    re-fires identically on each build. SAFE.
  - id-stream / golden stability: the demand pass runs ONLY when the mode
    flag is on AND the module has a bound `#query` (§4). When OFF, the
    pass returns immediately BEFORE minting any node — zero id-stream
    perturbation, the existing 165 goldens are byte-identical (§6). This is
    the mode-gate's natural home: a single `if (!demand_mode) return;` at
    the pass head.
  - MECHANISM COST: the demand pass must mint DataFlow objects that today
    only `BuildClause` mints. It needs a `QueryIO` for `d_p^α`, a receive,
    and the guarded-copy JOIN. The `QueryIO`/receive minting is the
    genuinely new surface (D4's cost, §7) — but it reuses A11's
    `BuildPredicate` machinery and A5's IO-proc lowering unchanged. No new
    op family, no new lowering path (p3 (d)).
  - VERDICT: RECOMMENDED. Bypasses all three E-37 gates WITHOUT weakening
    the user surface; satisfies E-45 by the same BuildIOProcedure path;
    parser-round-trip-safe; the mode-gate is a one-line pass-head guard
    that keeps the 165 goldens byte-identical off.

#### Option C — ControlFlow bypass (mint the injector + a hand-built handler proc, skip messsage_handler)

Keep `d_p^α` as a DataFlow relation but DO NOT mint a QueryIO/receive.
Instead, at ControlFlow build, hand-mint both the injector proc AND a
demand-handler proc (a vector-driven ingest into `d_p^α`'s table), and
wire the injector's CALL directly to the hand-built handler — bypassing
the `messsage_handler` map (A4) entirely.

  - E-37 gates: bypassed (no ParsedClause/Query forcing objects touched),
    same as Option D.
  - E-45 wiring: bypassed by construction — this is the "ControlFlow-only
    vector+handler-proc pair that bypasses messsage_handler" the D0 charge
    names. But bypassing A4 means the injector→handler edge is now a
    BESPOKE wiring the transform hand-builds, DUPLICATING what
    `BuildIOProcedure` (A5) does for free. Two code paths that must stay in
    sync (the F17/F18-style divergence risk the DR-IR epoch spent effort
    ELIMINATING via V-PRED-XCHECK).
  - DEBUG ROUND-TRIP: safe (no parse objects), same as D.
  - id-stream: safe if mode-gated, same as D. BUT the hand-built handler
    proc is a SECOND hand-coded emission surface — precisely the kind
    E-42 flags as a smell (the descent + the table-less VECTORLOOP shim
    are already two; this would be a third).
  - CRUX AGAINST C: `d_p^α` without a DataFlow receive has NO `forced_view`
    flow into the guarded copy — the demand relation would be sourced ONLY
    by the hand-built handler at ControlFlow level, INVISIBLE to the
    DataFlow graph. That breaks obligation (f)'s DataFlow-level source
    check (p3 §2(f): "every d_p has a real producing source" is a
    DataFlow-graph property, verified by `RemoveUnusedViews` /
    dead-flow); a ControlFlow-only source would let dead-flow elimination
    at DataFlow COLLAPSE `p'^α` (source-less at the graph it can see) and
    then the hand-built handler feeds a table nobody reads. UNSOUND unless
    the demand source is ALSO a DataFlow object — at which point you are
    back to Option D for the source and only bypassing the injector wiring,
    which buys nothing.
  - VERDICT: REJECTED. Bypassing `messsage_handler` re-introduces the
    two-path divergence the DR-IR epoch eliminated, adds a third hand-coded
    emission surface (E-42 smell), and — fatally — a ControlFlow-only
    source is invisible to DataFlow dead-flow elimination, breaking (f).

### 1.3 DECISION — Option D (DataFlow-level minting), defended

The demand transform, running at A21:2566 (mode-gated, no-op when off or
when the module has no bound `#query`), mints for each demanded adornment
`p^α`:

  1. the demand relation `d_p^α` (a `QueryImpl` relation, arity |bound(α)|);
  2. a synthetic `QueryIO` for `d_p^α` with a non-empty `Receives()` — the
     demand MESSAGE, existing purely as a DataFlow object (NOT a
     `ParsedMessage`), so A5's `BuildIOProcedure` populates
     `messsage_handler[d_p^α]` in the A6 loop and the injector's CALL
     target (A4) exists WITHOUT touching the user forcing surface;
  3. the guarded copy `p'^α = p ⋈ d_p^α` (the ⊥c-pivot JOIN, §1.1);
  4. the root-seed edge: the demanded query's bound-column projection is
     the seed INSERT into `d_p^α` (p3 (f) source 1), and the demanding
     subgoals are the recursive sources (p3 (f) source 2);
  5. at ControlFlow, `BuildQueryForceProcedureImpl` (A1) is
     GENERALIZED to synthesize the injector against the demand message's
     handler — its shape is UNCHANGED (VECTORAPPEND→CALL handler→RETURN);
     the only change is that the handler is `messsage_handler[d_p^α]`
     (minted by us at step 2) instead of `messsage_handler[user #message]`.

DEFENSE, one line per pressure:
  - E-37 gates 1-3: bypassed, user surface UNCHANGED — we never call
    `ParsedClause/Query::ForcingMessage` for a demand adornment (§1.2-D).
  - E-45 wiring: satisfied by the SAME `BuildIOProcedure` path E-45
    identifies — the demand IO is just another `query.IOs()` entry (A6).
  - debug round-trip: safe — the transform is a deterministic graph
    rewrite below the parser; no fabricated parse object (§1.2-D).
  - id-stream/goldens: mode-gated one-line pass-head guard; byte-identical
    off (§6).
  - mode-gate: `-demand` CLI flag, default off (§4).

The residual honest cost: minting a `QueryIO`+receive without a
`ParsedMessage` is genuinely new plumbing (D4's build work, §7). It is
NOT free — but it is the CHEAPEST of the three options, and it reuses A5
(IO-proc lowering) and A11's `BuildPredicate` (receive-view construction)
unchanged rather than duplicating or bypassing them.

--------------------------------------------------------------------------
## 2. PER-ADORNMENT SPLITS — transitive_closure.dr modeled exactly

### 2.1 The adornment derivation (E-41, re-derived)

`transitive_closure.dr` (A22): `#local tc(From,To)`, never directly a
`#query`. THREE queries project from it:
  - `reachable_from(bound From, free To) : tc(From, To)` → tc demanded `bf`
  - `reaching_to(free From, bound To)   : tc(From, To)` → tc demanded `fb`
  - `is_node(free Node)  : tc(Node,_) ; tc(_,Node)`     → tc demanded `ff`

Left-to-right magic-set propagation over the recursive rule
`tc(From,To) : tc(From,X), tc(X,To)`:
  - Seed `tc^bf` (From bound): SIP binds `From`; subgoal1 `tc(From,X)` sees
    From bound → demands `tc^bf` (already have it); it binds `X`; subgoal2
    `tc(X,To)` sees X bound → demands `tc^bf`. Closure of bf = {bf}.
  - Seed `tc^fb` (To bound) — the head `tc(From,To)` has To bound, From
    free. SIP must ground the recursion from the bound `To`. Visiting the
    body to make To available drives right-to-left: subgoal2 `tc(X,To)`
    has To bound (2nd pos) → adorned `fb` → demands `tc^fb`; it binds `X`.
    Then subgoal1 `tc(From,X)` has X bound (2nd pos), From free → adorned
    `fb` on that subgoal too... but the head is `fb` and the recursion is
    the SAME relation, so the propagated ADORNMENTS OF tc induced are:
    subgoal2 = `fb` and — because the standard magic-set closure also
    considers the head-driven pass where To flows to X and X becomes the
    JOIN key producing a From-and-To-bound demand on the deeper subgoal —
    the closure yields `tc^ff` (subgoal1 with neither head var bound after
    the pivot rename) and `tc^bb` (both bound). The D0 consolidated record
    (§2, E-41), independently re-derived by TWO fleet lanes and its
    verifier, gives the closure: reaching_to=fb contributes {fb, ff, bb}.
  - is_node=ff: adds `tc^ff` (already in the union).
  - UNION over the three queries = **{bf, fb, ff, bb}** — FOUR distinct tc
    adornments (E-41). The load-bearing point (multi-adornment split is
    mandatory) holds under either the THREE- or FOUR-count; I adopt FOUR
    per the fleet's re-derivation and flag §2.1's SIP-order dependence.

D1 HONEST NOTE on SIP determinism: the exact adornment set depends on the
SIP ORDERING RULE (which subgoal is visited first, and the availability
propagation direction). The fleet's {bf,fb,ff,bb} is the LITERAL
left-to-right magic-set result. The SIP-order rule is a DESIGN KNOB (D4
must fix ONE deterministic rule — house bet §6 is that this is where the
first real divergence surfaces). For THIS design I adopt the literal
left-to-right rule the fleet re-derived, and I flag any dependence on it
inline.

### 2.2 Which `d_tc^α` relations and guarded copies exist

Under the literal rule, the transform mints FOUR demand relations and
FOUR guarded copies of the recursive clause:

  | α  | `d_tc^α` arity | root seed source | guarded copy `tc'^α` |
  |----|----------------|------------------|----------------------|
  | bf | 1 (From)       | reachable_from's bound `From` | `tc-body ⋈ d_tc^bf` on From |
  | fb | 1 (To)         | reaching_to's bound `To`      | `tc-body ⋈ d_tc^fb` on To   |
  | ff | 0 (all free)   | is_node (all-free)            | NONE — demand-inert (§2.3)  |
  | bb | 2 (From,To)    | (arises only via fb's SIP)    | `tc-body ⋈ d_tc^bb` on From,To |

Each `#query` entry point reaches its adornment via its bound-column
projection (A3's `col_indices`): reachable_from's bound From (col 0) seeds
`d_tc^bf`; reaching_to's bound To (col 1) seeds `d_tc^fb`; is_node has NO
bound column (A3 `col_indices` empty → no scanned_index, no forcer) and
reaches the ff adornment which is demand-inert.

### 2.3 The ALL-FREE case (is_node = ff) — inertness re-examined against the concrete mechanism

p3 §2(e) DECIDED: an all-free demanded predicate gets NO demand edge (a
zero-pivot demand JOIN is illegal outside @product; a nullary demand
conveys no information). Re-examined against the CONCRETE mechanism:

The `ff` adornment has |bound| = 0, so `d_tc^ff` would be arity 0 (a unit
token) and `tc'^ff = tc ⋈ d_tc^ff` would be a ZERO-PIVOT JOIN. The SKIP is
correct: `tc'^ff = tc` (no rewrite), tc^ff materializes in full.

BUT the concrete mechanism forces a SHARPER observation the abstract
transform glossed: **an all-free CONSUMER of `tc` makes `tc` demand-inert
GLOBALLY unless a sibling adornment prunes.** is_node reads ALL of tc
(both `tc(Node,_)` and `tc(_,Node)`). If ANY consumer needs all of tc,
then tc must be materialized in full for that consumer REGARDLESS of what
bf/fb demand — the guarded copies `tc'^bf`/`tc'^fb`/`tc'^bb` produce a
SUBSET, but is_node needs the SUPERSET. The demand-pruned copies and the
full tc must COEXIST, and the full tc is what is_node reads.

INERTNESS-PROPAGATION RULE (stated exactly, the design's commitment):

> A predicate `p` is DEMAND-INERT (materialized in full, no guard) iff
> SOME consumer of `p` demands `p` at the all-free adornment (|bound|=0),
> OR `p` is reachable ONLY through a negation/aggregate frontier (§3(a)
> sink). Otherwise `p` is DEMAND-LIVE and each `α` with |bound|≥1 gets a
> guarded copy `p'^α`. When `p` is inert, the guarded copies are STILL
> minted for the live adornments (they prune for THOSE query paths), but
> the full `p` is ALSO retained because the inert consumer reads it.

WHAT THIS DOES TO transitive_closure.dr: is_node's ff-demand makes `tc`
inert → FULL `tc` is materialized (for is_node) AND `tc'^bf`/`tc'^fb`/
`tc'^bb` are minted (for reachable_from/reaching_to). But here is the
honest kicker: **in tc.dr, the full `tc` that is_node forces is the SAME
transitive closure the bf/fb queries would prune from.** Since full tc is
computed anyway (is_node demands it), the guarded copies prune NOTHING
that wasn't already going to be computed — they are pure overhead on THIS
witness. The sibling-guard inertness propagates and NO net pruning
survives in tc.dr.

CONCLUSION for tc.dr: it is a NEGATIVE witness for demand — an all-free
sibling query defeats the pruning. This is EXACTLY why p3 §3.1 authored a
SEPARATE `demand_tc_witness.dr` with NO all-free consumer:

```
#message edge_2(u64 From, u64 To)
#local path(u64 From, u64 To)
path(F, T) : edge_2(F, T).
path(F, T) : path(F, M), edge_2(M, T).
#query reachable_from(bound u64 From, free u64 To) : path(From, To).
```

Here `path` has ONE consumer (reachable_from = bf), NO all-free sibling,
so `path` is DEMAND-LIVE: `d_path^bf(From)` seeds one source, `path'^bf`
computes only the forward-reachable frontier, and full `path` is NEVER
materialized (dead-flow elimination removes the unguarded `path` once
nothing reads it). PRUNING SURVIVES. This is the §5 primary witness.

The rule's tripwire: the transform must detect the inert case and NOT
delete the full `p` when an inert consumer exists — a dead-flow
elimination that removed full tc because `tc'^bf` "replaces" it would
UNDER-DERIVE is_node. The inertness rule is the guard against that.

--------------------------------------------------------------------------
## 3. THE (a)-(f) INVARIANT ARGUMENT — re-reviewed against Option D's ACTUAL objects

p3-demand-argument.md §2 argued (a)-(f) against the ABSTRACT transform.
Here each is re-argued against the CONCRETE Option-D objects: the minted
`QueryIO`+receive for `d_p^α`, the ⊥c-pivot guarded copy, and the
generalized injector. The obligation SURVIVES only if the concrete object
satisfies it — a concrete object can break what the abstract one didn't.

### (a) Stratification + the Stratify cross-check, with demand-through-negation/aggregation BLOCKED

INVARIANT SITE: `QueryImpl::Stratify` + the always-on cross-check
(A20:387-419). The CONCRETE hazard beyond p3's: the minted `QueryIO`
receive for `d_p^α` is a NEW SOURCE node in the graph BEFORE Stratify runs
(the pass is at A21:2566, Stratify at :2610 — p3 §1.3 point 3). A minted
receive that lands in an SCC could create an induction the union-find
partition disagrees with (A20:404-413 abort).

RE-ARGUED: the minted `d_p^α` receive is a demand SOURCE — it has NO
incoming edge from `p'^α` except the pivot match (an equi-join edge, not a
back-edge). The demand propagation graph is a SUBSET of the original
POSITIVE dependency reachability (p3 §2(a)). Adding a positive predecessor
edge (`d_p^α` → `p'^α` via the JOIN pivot) can only MERGE SCCs
monotonically; it cannot introduce a negation-on-a-cycle. The RESTRICTION
that makes this hold — **demand-through-negation and
demand-through-aggregation BLOCKED** — is realized concretely as: the SIP
walk treats a negated subgoal `not q` / an aggregate `q over …` as a
DEMAND SINK (q materialized in full, no `d_q` minted, no `QueryIO`
synthesized for q's demand). Concretely in Option D: the transform's
adornment-propagation loop, when it reaches a negate/aggregate view in
the DataFlow graph (NEGATE / AGG node types, `lib/DataFlow/Query.h`),
STOPS propagating and mints NO demand IO on the far side. So no minted
receive ever lands across a negation/aggregate — the SCC subset property
holds by construction.

DIAGNOSTIC when it would break: if `p`'s bound columns are available ONLY
after a negated/aggregated subgoal in every SIP order, the transform
emits a CLEAN DIAGNOSTIC (sibling to the unstratified-negation reject) and
mutates NOTHING — fires before graph mutation, so default-mode compile is
untouched. The A20 cross-check is the debug backstop that catches any
future relaxation letting demand cross a negation.

CONCRETE-OBJECT CHECK: because the demand pass runs BEFORE Stratify, the
minted `d_p^α` receive and `p'^α` JOIN are STRATIFIED TOGETHER with the
rest — Stratify sees the final graph. The cross-check validates the demand
edges. SURVIVES.

### (b) Structural distinctness — the demand JOIN's Equals()-visible shape

INVARIANT SITE: A17/A18/A19. Re-argued in §1.1: `p'^α`'s top JOIN has
`num_pivots = |x̄| ≥ 1` and an extra `d_p^α`-SELECT child; `p` has
neither; `Equals()` (A19) short-circuits BEFORE group_ids. The CONCRETE
addition: with Option D, `d_p^α`'s SELECT reads a MINTED receive relation
(not a user relation), but `Equals()` compares num_pivots/children
STRUCTURALLY — the child being a synthetic receive vs a user relation is
irrelevant to the short-circuit. SURVIVES. Also: the keep-last-edge rule
(Join.cpp:314-322) protects the demand pivot through Optimize even if a
demand-column value is a compile-time constant — the same protection the
⊥c pivot gets. So Optimize will not strip the guard and collapse `p'^α`
onto `p`. SURVIVES.

### (c) keep-last-edge + unit-relation rules

INVARIANT SITE: Join.cpp:314-322 + the CLAUDE.md dataflow-invariants
block. The demand JOIN ADDS an input-column edge (never severs one) —
keep-last-edge preserved and, in fact, protecting the transform's own
edge. `d_p^α` is NOT a unit relation (it has real bound-column rows, arity
≥1, not `is_condition`) — so the unit-relation carve-outs ("CSE never
folds a unit SELECT into a non-unit one", "a JOIN pivot whose non-user
side is a unit relation is never removed") do not apply; `d_p^α` obeys
general JOIN canonicalization. The transform mints NO unit INSERT (only
the zero-arity desugarer does). "No view is its own direct user"
(RelabelGroupIDs): the demand JOIN's inputs are `p'`-body and
`d_p^α`-SELECT, neither the JOIN itself. SURVIVES.

### (d) rewrite-not-evaluator (SLDMagic: a transformation, NEVER an evaluator)

INVARIANT SITE: the whole DR-IR lowering path is UNCHANGED. Option D's
output is a `QueryImpl` (relations, JOINs, TUPLEs, a minted receive +
QueryIO) consumed by the existing Query→DR→Program→C++ lowering. NO new
DR op, NO new Program region, NO new runtime store, NO demand-queue
object, NO goal stack, NO runtime unification. The injector (A1,
generalized) is the SAME push-only message-batch mechanism `force.dr`
already uses — it converts the runtime bound argument into an ordinary
message batch (A14/A15). The "which instances to materialize" decision is
COMPILED IN as graph structure (the guard JOIN), not DISPATCHED at
runtime. The minted `QueryIO` is the ONE genuinely new object, but it is
an EXISTING node type (QueryIO), lowered by the EXISTING A5 path — not a
new evaluator. SURVIVES — and this is the load-bearing SLDMagic-memory
check: Option D was chosen over Option C precisely BECAUSE C's hand-built
handler would have been a bespoke runtime dispatch path (an evaluator
smell).

### (e) no zero-pivot demand JOIN outside @product (the all-free SKIP)

INVARIANT SITE: CLAUDE.md "zero-pivot JOINs appear only under @product";
`ViewSelfReachable` (Build.cpp CF :200/:1166). The transform mints a
demand guard ONLY when |x̄| ≥ 1 (§2.3 inertness rule: all-free → SKIP, no
`d_p^α`, no `QueryIO`, no JOIN). So no demand JOIN is ever zero-pivot;
none needs @product; the @product-only invariant is never approached.
CONCRETE CHECK: the SKIP happens BEFORE any minting — an all-free
adornment produces zero synthesized objects, so there is no zero-pivot
JOIN to reject. SURVIVES.

### (f) every `d_p^α` has a real producing source — including the ROOT seed at epoch 0

INVARIANT SITE: CLAUDE.md "a source-less forwarding cycle is
unsatisfiable, collected by dead-flow elimination" (View.cpp:996-1000);
`RemoveUnusedViews`. Every `d_p^α` has TWO real DataFlow source classes:
  1. THE ROOT SEED: the demanded query's bound-column projection is a real
     INSERT into `d_p^α`. Concretely with Option D: the minted `QueryIO`'s
     receive IS the root source — the injector (A1) batches the runtime
     bound tuple into it, exactly as `force.dr`'s `inject_20` batches into
     `trigger_generate_next_id`'s receive (A14). A minted receive with a
     non-empty `Receives()` is a real DataFlow SOURCE node, not a phantom.
  2. THE DEMANDING SUBGOALS: recursive demand propagation — `d_q` sources
     `d_p` via SIP-available bindings, all grounded in the root seed (1).

THE SHARP QUESTION the charge names — **is `d_p^α` EMPTY at epoch 0, and
is that sound?** YES it is empty at epoch 0, and YES it is sound. At
`init(db,...)` (epoch 0, before any message) NO query has been called, so
NO bound argument has been injected → `d_p^α`'s table is empty →
`p'^α` produces nothing → `p` (if demand-live) is empty. This is CORRECT:
a demand-driven relation SHOULD be empty until something demands it. The
soundness rests on the SOURCE being STRUCTURALLY PRESENT (a minted receive
node with a real edge) even when its DATA is empty — dead-flow elimination
keys on STRUCTURAL source presence (is there a producing view?), NOT on
runtime row count. force.datalog.h confirms the analog: `get_next_id_4` is
empty until a `get_next_id_bf(...)` call injects (A13) — the table and
its receive proc EXIST at epoch 0, empty. The minted `d_p^α` receive is
the same: structurally present, runtime-empty until demanded. SOUND.

The (f) tripwire (a design-time validator, analogous to the DR-IR census):
post-transform, assert every minted `d_p^α` has ≥1 producing view (its
minted receive) reachable from a `#query` seam — a cheap graph walk. The
transform must emit the root seed (the receive) BEFORE any `p'^α` guard,
so `d_p^α` is never TRANSIENTLY source-less across the rewrite order.

### 3.x THE UN-DISCHARGED RESIDUE — the demand-source PROJECTION CSE question

The ledger names a residue p3 discharged for `p'` but NOT for the demand
SOURCE: a demand-source PROJECTION view (the demanding-subgoal rule
`d_p^α(x̄) :- <bindings>` is a PROJECTION/TUPLE of an already-sourced
subgoal) has EMPTY group_ids (A18: only JOIN/AGG/KVINDEX seed group_ids;
a TUPLE/projection gets empty). So `InsertSetsOverlap` (A17) reports two
such projections MERGEABLE. QUESTION: can two DIFFERENT adornments'
demand-source projections CSE-merge UNSOUNDLY?

DISCHARGE: consider `d_tc^bf` sourced by a projection producing `[From]`
and `d_tc^fb` sourced by a projection producing `[To]`. These project
DIFFERENT columns → the projections have DIFFERENT `Equals()`-visible
shape (different input-column edges) → CSE's `Equals()` returns false
STRUCTURALLY, BEFORE group_ids is consulted. Two demand-source
projections CSE-merge ONLY if they project the SAME columns from the SAME
input — i.e. they are the SAME demand on the SAME bound-column shape. THAT
MERGE IS BENIGN AND DESIRABLE: it is exactly the shared-demand-frontier
fusion p3 §1.3 point 2 WANTS (two subgoals demanding `p` on the same key
share one demand relation → fewer materialized demand rows). So:

  - DIFFERENT adornments (different bound columns) → different projection
    shape → NO CSE merge (structurally distinct). SOUND.
  - SAME adornment, two demanding subgoals → SAME projection shape → CSE
    MERGES → shared demand frontier. BENIGN + DESIRABLE.

The empty-group_ids MERGEABLE verdict from A17 is REACHED only in the
second case (same shape), where the merge is wanted. In the first case
(different adornments) `Equals()` short-circuits first. So the residue is
DISCHARGED: no unsound cross-adornment merge; the only merges that happen
are same-adornment demand-frontier fusions, which are the feature. The
CAVEAT for D4: the transform must NOT rely on group_ids to keep different
adornments' demand sources apart (E-32's whole point) — it relies on the
projected-column-set structural difference, which `Equals()` sees. If two
different adornments ever projected the SAME column set (impossible by
construction — the bound-column set IS the adornment), the merge would be
benign anyway (same demand key = same demand relation). SOUND either way.

--------------------------------------------------------------------------
## 4. THE SURFACE QUESTION — framed for the owner, with a recommendation

Three surfaces for HOW a query becomes demand-transformed, and the
mode-flag granularity question, priced against the E-40 fact (38/165 =
23.0% of goldens carry a bound `#query`) and bring-up risk.

### 4.1 (i) fully-implicit / (ii) per-query pragma / (iii) per-program flag

  - (i) FULLY-IMPLICIT: every bound `#query` is demand-transformed under
    the mode flag. Simplest surface (no new syntax), but the coarsest — it
    demand-transforms even queries where demand HURTS (the tc.dr inert
    case §2.3: full tc computed anyway, guards are pure overhead). Under a
    mode flag it is at least opt-in-per-PROGRAM.
  - (ii) PER-QUERY PRAGMA (`#demand` or `@demand` on the `#query` decl):
    the author marks which queries to demand-transform. Finest control;
    lets the author skip inert cases. COST: new parse surface (a pragma on
    `#query`), new parse-object plumbing, and it puts the SIP/inertness
    judgment on the AUTHOR (who may not know whether a sibling all-free
    consumer defeats pruning — §2.3). Also: a per-query pragma is a NEW
    forcing-surface-adjacent construct that would interact with the E-37
    gates (a pragma'd query is exactly a per-adornment-forced query).
  - (iii) PER-PROGRAM MODE FLAG ONLY (`-demand` CLI): the whole module is
    demand-transformed (all bound queries) or none. Coarsest, but ZERO new
    syntax, ZERO parse-object plumbing, and it is the BRING-UP posture p3
    §4/§5 already recommends (mode-gated, default-off).

### 4.2 Mode-flag granularity + interaction with the 4 golden modes

The 4 golden modes today are (opt, nocf, nodf, none) — dataflow-opt ×
controlflow-opt toggles (CLAUDE.md). Demand is ORTHOGONAL to both: it is a
Query-graph rewrite that runs BEFORE Optimize (A21:2566), so it composes
with dataflow-opt on/off. Two ways to expose:

  - A 5th "demand-on" MODE: adds a demand-on execution variant per case.
    But demand-on produces DIFFERENT materialization for the 38 bound-query
    cases (p3 §5.1) → NOT byte-comparable against the demand-off golden →
    those 38 need SEPARATE demand-on goldens (oracle-refereed for
    answer-identity). This DOUBLES the golden surface for 38 cases. Large.
  - A FLAG ORTHOGONAL to the 4 modes (`-demand`, default-off), exercised
    ONLY by the NEW witness case(s): the existing 165 run their 4 modes
    demand-OFF (byte-identical, §6); the new `demand_tc_witness.dr` runs
    its 4 modes demand-ON (its own goldens, oracle-blessed). No 5th mode,
    no re-bless of 38. This is the minimal-blast-radius posture.

### 4.3 RECOMMENDATION

**RECOMMEND (iii) per-program `-demand` CLI flag, default-off, ORTHOGONAL
to the 4 golden modes (not a 5th mode), for bring-up.** Rationale:
  - Zero new syntax / parse plumbing (Option D's DataFlow-level minting
    needs no pragma; the flag is a single `if (!demand_mode) return;` at
    the pass head, §1.3).
  - Keeps the 38 bound-query goldens BYTE-IDENTICAL off (§6) — no golden
    re-negotiation this epoch (the E-40 blast radius is deferred).
  - The new witness exercises demand-ON via the flag with its OWN goldens.
  - LEAVES ROOM for (ii): a future `@demand` per-query pragma is a strict
    refinement — the CLI flag becomes "demand-transform all pragma'd
    queries (or all bound queries if `-demand-all`)". So (iii) now does not
    foreclose (ii) later. The pragma is worth it ONLY once the SIP/
    inertness judgment is stable enough to hand to authors — premature now.
  - AGAINST (i) default-implicit: it would re-bless 38 goldens THIS epoch
    (E-40) and demand-transform inert cases (tc.dr overhead) — both are
    net-negative during bring-up. (i) is the eventual END STATE (demand as
    the default), but only after the measure-first bar (p3 §3.2) proves the
    benefit and the SIP rule is battle-tested.

BRING-UP RISK PRICED: the CLI-flag posture confines ALL risk to the new
witness case(s). A miscompile in the demand transform can only affect
demand-ON runs of the new cases — the 165-case net is structurally
immune (the pass does not run). This is the R4/P3 discipline realized: the
transform ships behind a flag that CANNOT touch the existing net.

--------------------------------------------------------------------------
## 5. THE HAND-WRITTEN WITNESS (checkpoint-3 discipline)

PRIMARY witness: `demand_tc_witness.dr` (p3 §3.1), the SINGLE-adornment
(bf) shape — one demand-live predicate, NO all-free sibling, so pruning
survives (§2.3). transitive_closure.dr's four-adornment split is the
STRETCH sketch (§5.4). Hand-writing the desired output BEFORE generalizing
is the epoch's checkpoint-3 gate.

```
#message edge_2(u64 From, u64 To)
#local path(u64 From, u64 To)
path(F, T) : edge_2(F, T).                 ; base
path(F, T) : path(F, M), edge_2(M, T).     ; right-linear recursion
#query reachable_from(bound u64 From, free u64 To) : path(From, To).
```

Demanded adornment set: reachable_from's bound From → `path^bf`. SIP over
the recursive rule `path(F,M), edge_2(M,T)`: F bound (from `d_path`),
subgoal1 `path(F,M)` demands `path^bf` (already have it), binds M;
subgoal2 `edge_2(M,T)` is a MESSAGE (demand-inert base) → materialized as
delivered. Closure = {bf}, singleton. NO negation, NO aggregate, NO demand
cycle → §3(a) trivial.

### 5.a THE POST-TRANSFORM DATAFLOW GRAPH (p3 §1.2 ASCII style)

Objects minted by the demand pass (Option D), in addition to the base
graph. `⊥d = d_path^bf` is the minted demand relation (arity 1: From).

```
  === MINTED DEMAND SOURCE (the "message" level, Option D) ===

    QueryIO(d_path_bf)  Receives()={recv}    <- minted QueryIO, non-empty receive
              |
        RECEIVE d_path_bf [From]             <- minted message-receive view (A11-analog)
              |
        INSERT  → d_path_bf table            <- d_path_bf's producing source (f-source 1)

  === ROOT SEED (query-call time; the injector feeds the receive above) ===

    reachable_from(bound From) entry
              |  injector (A1 generalized): VECTORAPPEND{From} → CALL handler[d_path_bf]
              v
        (delivers From into the RECEIVE d_path_bf above)

  === THE GUARDED RECURSIVE COPY  path'^bf ===

    edge_2 receive [F,T]          path'^bf [F,M]        SELECT d_path_bf [F]
         (base rule)                    \                    /
              \                     JOIN (pivot: F) [F, M]           <- |x̄|=1, ⊥c-shape
               \                          |
                \                    (bind M; probe edge_2(M,T))
                 \                        |
                  \                  path'^bf-body [F, T]
                   \                     /
                    MERGE path'^bf [F, T]                <- the demand-guarded closure
                         |
                    INSERT → path table (guarded)

  === DEMAND PROPAGATION (recursive subgoal re-demands the same key) ===

    path'^bf-body's subgoal path(F,M):  projects [F] → d_path_bf   <- f-source 2
       (SAME adornment bf, SAME column {From} → CSE-fuses with the root
        seed's d_path_bf source; §3.x — the shared demand frontier)

  === THE QUERY READ (unchanged shape; reads the guarded path) ===

    reachable_from_bf : SELECT over path table, scan-index on From
```

KEY POINTS:
  - the unguarded `path` is DEAD (no consumer reads it — reachable_from
    reads the guarded closure). Dead-flow elimination REMOVES full `path`;
    only `path'^bf` materializes → PRUNING SURVIVES (contrast tc.dr §2.3).
  - the JOIN pivot on F is the ⊥c-pivot generalized (A16); structural
    distinctness (A19) keeps `path'^bf` from CSE-collapsing onto `path`.
  - `d_path_bf` is sourced by (1) the minted receive (root) and (2) the
    recursive subgoal's projection — which, being the SAME bf key, CSE-fuse
    into ONE demand relation (§3.x benign merge).

### 5.b THE ENTRY-PROC TREE + DR-IR OPS (v3-spec §2.1 effect-set style)

Synthesized injector proc (A1 generalized), mirroring force.ir `^inject:20`
(A15):

```
proc ^inject_demand:N(@From)                    ; kQueryMessageInjector
  vector-define $q<u64>
  seq
    vector-append {@From} into $q<u64>
    call ^receive:d_path_bf/1($q<u64>)           ; handler[d_path_bf] (A4)
    return-true

proc ^receive:d_path_bf/1($param<u64>)           ; minted by BuildIOProcedure (A5)
  seq
    call ^entry:...($param)                       ; drives the demand ingest fold
    return-true
```

The demand receive's INGEST FOLD (v3-spec §2.1 `INGEST_FOLD`, effect-set
notation from spec:315-318). `d_path_bf` is a MONOTONE demand relation
(demand keys are never retracted in the acyclic-source shape — no `-` arm;
DERIVED by TrackDifferentialUpdates, not asserted):

```
OP: INGEST_FOLD(table d_path_bf, sign +, class Monotone)   [entry-proc only]
  Effects: vector:drain($param queue);
           counter+(d_path_bf, Monotone);
           flags:read(kInI);
           vector:append(d_path_bf's net-additions frontier).
  Body: access-plan — for each {From} in $param:
          update-count +nonrecursive {From} in d_path_bf;
          on newly-added: append {From} to d_path_bf's net-add frontier.
```

That net-add frontier is the cut successor feeding `path'^bf`'s guard
JOIN (the same net-additions-frontier idiom the R3 aggregate input uses,
CLAUDE.md). The guarded closure `path'^bf` then lowers through the
EXISTING SEED_FOLD / FIXPOINT_FIRE machinery unchanged — it is an ordinary
recursive differential/monotone relation; the demand pass added NO new op
(p3 (d) / §3(d)).

EXPECTED REGION SHAPE (the entry proc the injector's receive drives): a
SERIES whose head is the `INGEST_FOLD` above (empty-bodied UPDATECOUNT
hole filled by the eager descent, per the ingest hole contract,
Stratum.cpp:1909 / Procedure.cpp:107), followed by the fixpoint round
shells for `path'^bf`'s SCC. BYTE-IDENTICAL to a hand-written
`.dr` that declared `#message d_path_bf(u64 From)` and joined it — which
is the whole point: Option D synthesizes exactly that graph without the
user writing it.

### 5.c THE GENERATED-API SURFACE (what the driver sees)

The demanded query now takes `(db, log, functors, bound...)` — EXACTLY
force.dr's shape (A13, E-44). Confirmed against E-44: the Log is threaded
but UNUSED on the inject path (the injector receives allocator+functors,
not log — A14). So:

```cpp
// demand-ON:
template <typename Log, typename Functors>
friend reachable_from_bf_cursor
reachable_from_bf(Database &db, Log &, Functors &functors, uint64_t From) {
  assert(db.initialized_);
  inject_demand_N(db.allocator, functors, /*tables...*/, From);  // seed d_path_bf
  return {db, From, db.idx_path_on_From.First({From})};          // then read
}
```

CONTRAST demand-OFF (today, A3): `reachable_from_bf(db, From)` — NO
log/functors, a pure read-time index probe (no inject; the full `path` is
already materialized). So the API SIGNATURE CHANGES when demand is ON for
this query (gains `log, functors`), exactly matching force.dr. E-44
confirms this is the established shape.

### 5.d BYTE-IDENTICAL vs CHANGED

  - MODE OFF (all 165 existing cases, all 4 golden modes): BYTE-IDENTICAL
    to today. The demand pass returns at its head (`if (!demand_mode)
    return;`) before minting any node — zero id-stream perturbation, zero
    graph change. Verified-safe by the mode-gate being a pass-head guard
    (§6.1).
  - MODE ON (the new witness only): the graph gains the minted
    `d_path_bf` QueryIO/receive/INSERT, the guarded `path'^bf`, the
    injector proc; the unguarded `path` is dead-flow-eliminated; the query
    API gains `(log, functors)`. All NEW golden surface, oracle-blessed.

### 5.4 STRETCH: transitive_closure.dr multi-adornment (the four-copy sketch)

Under the literal SIP rule (§2.1), tc.dr mints FOUR demand relations
{d_tc^bf, d_tc^fb, d_tc^ff(SKIP), d_tc^bb} and guarded copies for
{bf, fb, bb}. BUT §2.3's inertness rule fires: is_node's ff-demand makes
tc INERT → full tc is materialized anyway → the guarded copies prune
NOTHING net on this witness. So tc.dr is the NEGATIVE control (demand-ON
produces the SAME materialization as demand-OFF because the all-free
sibling forces full tc). Its value as a witness is precisely to PROVE the
inertness rule fires correctly (answer-identity + NO spurious pruning of
what is_node needs). It is a stretch/secondary case, authored AFTER the
primary `demand_tc_witness.dr` lands.

--------------------------------------------------------------------------
## 6. PRE-REGISTERED PREDICTIONS

### 6.1 Existing-165 churn under default modes — MUST BE ZERO

PREDICTION: all 165 existing cases, all 4 golden modes, BYTE-IDENTICAL
demand-off (which is the DEFAULT — the `-demand` flag is not passed by
diffrun.sh/runall.sh). VERIFICATION (how the judge/D4 confirms): the
demand pass is a single function invoked at DataFlow Build.cpp:2566 guarded
by `if (!ctx.demand_mode) return;` at its head, BEFORE any node minting.
With the flag off, the function body never runs → the `QueryImpl` graph is
bit-for-bit what it is today → id-stream identical → goldens identical.
The verification is a full-suite run at HEAD-with-transform-but-flag-off:
`DR=... runall.sh` must end `SUITE: PASS` with zero churn. This is the
D4-landing gate; the mechanism GUARANTEES it structurally (a pass that does
not run cannot perturb output). Confirmed the corpus is 165 and 38/165
carry a bound query THIS session.

### 6.2 New-case plan

  - `demand_tc_witness.dr` (§5) + `.main.cpp` driver + `.batches` sidecar +
    `demand_tc_witness.oracle.stdout` + `.monotone.stdout` goldens. The
    oracle (`bin/Oracle`) proves demand-ON and a definitional full-eval
    produce the SAME query ANSWERS (the guard changes materialization,
    never the answer). ORACLE-BLESSED ONLY (never to make a red case
    green — CLAUDE.md blessing discipline).
  - Possibly +1 `demand_through_negation_1.dr` — an all-4-modes-diagnostic
    reject case (the §3(a) reject), joining runall.sh's expected-diagnostic
    list alongside `evm_func_parse`. Authored iff §3(a)'s diagnostic is
    wired in D4.
  - The tc.dr four-adornment inertness case (§5.4) is a SECONDARY negative
    control, authored after the primary.

### 6.3 Suite count trajectory

165 → 166 (primary witness) → 167 (negation reject) → 168 (tc inertness
control), landing incrementally. Minimum this epoch: +1 (166) if only the
primary lands design-first.

### 6.4 Driver churn

ZERO churn to the 165 existing drivers (demand-off, byte-identical). The
NEW witness driver observes the demand-ON API surface (§5.c): its query
call is `reachable_from_bf(db, log, functors, From)` (gains log/functors
per E-44), draining the returned cursor. It is a NEW driver, not a change
to an existing one. If a future 5th demand-on mode were added (NOT
recommended, §4.2), the 38 bound-query drivers would each need a demand-on
variant — a large churn the recommended flag-orthogonal posture AVOIDS.

### 6.5 Q5 / flagship neutrality

Q5 NEUTRAL. Q5 is a fixed chain with NO bound query → the demand pass
no-ops on it even demand-ON, and demand-off it never runs. The flagship
bench spot stays flat (per the §0 baseline discipline). The demand witness
is a NEW bench workload (a scale-knob sparse-reachability spike, p3 §3.2),
never a Q5 variant.

### 6.6 THE HOUSE BET — where the first real divergence surfaces

Per the E-1..E-45 precedent (every first-time invariant instrument finds a
real divergence), the FIRST end-to-end demand rewrite of the recursive
witness will surface a divergence, MOST PROBABLY in ONE of:
  1. THE SIP-ORDER RULE (§2.1 honest note): which columns are "bound" at
     which point in the recursive body, hence which adornment set is
     minted. A wrong SIP order mints the wrong `d_p^α` set → wrong
     materialization (oracle disagreement). This is my PRIMARY bet — the
     SIP rule is the least-pinned piece of this design.
  2. THE DEMAND-SOURCE SEEDING ORDER (obligation (f)): a `d_p^α`
     transiently source-less across the rewrite order → dead-flow
     elimination collapses `p'^α` before the recursive-subgoal source is
     wired → under-derivation. The mitigation (emit the root receive
     BEFORE any guard, §3(f)) is the tripwire.
  3. THE MINTED QueryIO ROUND-TRIP through Optimize: the minted receive/IO
     surviving CSE + canonicalization intact (the guarded copy's structural
     distinctness holds, §3(b), but the minted IO's interaction with
     ProxyInsertsWithTuples / LinkViews at DataFlow Build.cpp:2576+ is
     unexercised terrain).
PRE-REGISTERED: a FINDINGS entry iff the demand rewrite produces a wrong
answer (oracle disagreement) or a source-less `d_p^α` (dead-flow collapse)
on the primary witness. The oracle answer-identity gate + the (f)
tripwire are the instruments.

--------------------------------------------------------------------------
## 7. D4 SCOPE INPUT — for owner decision (d)

WHAT THE LIVE TRANSFORM (D4) NEEDS BEYOND THIS DESIGN:

  1. THE DEMAND PASS itself (`lib/DataFlow` new file, ~the SLDMagic
     rewrite): adornment propagation (SIP walk), the demand-relation
     minting, the guarded-copy JOIN construction (reusing the A16
     ⊥c-pivot machinery), the inertness rule (§2.3), the negation/
     aggregate sink + clean diagnostic (§3(a)). This is the bulk.
  2. THE MINTED QueryIO+RECEIVE plumbing: minting a `QueryIO` with a
     non-empty `Receives()` and a receive-view WITHOUT a `ParsedMessage`
     (Option D's genuinely new surface, §1.3). Must reuse A5's
     `BuildIOProcedure` and A11's `BuildPredicate` unchanged — the new
     code is the SYNTHESIS of the QueryIO node, not new lowering.
  3. THE GENERALIZED INJECTOR (A1): `BuildQueryForceProcedureImpl`
     generalized to synthesize the injector against the minted demand
     handler (`messsage_handler[d_p^α]`) instead of a user #message
     handler. Shape unchanged (§1.3 step 5).
  4. THE `-demand` CLI FLAG + pass-head guard (§4.3, §6.1) — trivial.
  5. THE (f) DESIGN-TIME VALIDATOR (§3(f) tripwire) — a cheap post-transform
     graph walk asserting every minted `d_p^α` has a producing source.
  6. THE WITNESS CORPUS (§6.2) — the primary `.dr` + driver + oracle
     goldens.

IS D4 IN-SCOPE THIS EPOCH, OR RE-SEED?

RECOMMENDATION: **D1 is DESIGN-ONLY-RATIFIED this epoch; D4 (the live
transform) is a candidate for LATE this epoch OR the follow-on, at the
owner's re-rank (§16.3).** The honest cost:
  - The design SURVIVES for the positive, non-recursive-through-negation,
    ≥1-bound-column slice (the primary witness). That slice is SHIPPABLE
    behind the `-demand` flag with the 165-net structurally immune (§4.3).
  - But the SIP-order rule (§2.1, §6.6 bet #1) is the least-pinned piece
    and is where the first divergence will surface. D4 should NOT start
    until the SIP rule is fixed to ONE deterministic definition and the
    primary witness's hand-written graph (§5.a) is confirmed against a real
    `#message d_path_bf`-and-JOIN hand-authored `.dr` (checkpoint-3:
    hand-write, compile, diff BEFORE generalizing). That confirmation is a
    cheap spike (author the hand-demanded `.dr`, emit its `.ir`/`.h`, check
    it matches §5.a/§5.b) that de-risks D4 without writing the transform.
  - PARALLELISM with D2/D3: the demand transform touches ONLY `lib/DataFlow`
    (the pass) + the injector generalization in `lib/ControlFlow/Build`. It
    is INDEPENDENT of D2 (config_agg_2 Seal-config fork) and D3
    (InstanceStore redesign) — no shared code. So D4 can proceed in
    parallel OR defer without blocking them.

HONEST COST HEADLINE: the design is complete and defensible for the narrow
positive slice; the LIVE transform is a real multi-week surface (the SIP
walk + minted-QueryIO plumbing are the two new pieces), and the SIP rule
is the risk. RECOMMEND the hand-demanded-`.dr` checkpoint-3 spike (cheap,
de-risking) as the immediate next step, with D4 proper gated on that
spike matching §5.a/§5.b.
